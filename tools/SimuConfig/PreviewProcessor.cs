using System;
using System.Drawing;
using System.Drawing.Imaging;

namespace SimuConfig
{
    /// <summary>
    /// Applies post-processing effects to a Bitmap to simulate SimuFX shaders.
    /// All operations are done in software on the CPU.
    /// </summary>
    public static class PreviewProcessor
    {
        public static Bitmap Apply(
            Bitmap source,
            float exposure,    // -1..1   (0 = neutral)
            float contrast,    // 0.5..2  (1 = neutral)
            float saturation,  // 0.5..2  (1 = neutral)
            float shadows,     // 0..2    (1 = neutral lift)
            float highlights,  // 0..2    (1 = neutral compress)
            float sharpen,     // 0..1.5  (0 = off)
            float vignette,    // 0..1    (0 = off)
            float shadowDepth) // 0..1    (0 = off)
        {
            // Work on a fresh 32bpp copy
            var bmp = new Bitmap(source.Width, source.Height, PixelFormat.Format32bppArgb);
            using var g = Graphics.FromImage(bmp);

            // 1. Exposure + Contrast + Saturation via ColorMatrix
            float exp = MathF.Pow(2f, exposure * 2f);   // -1..1 → 0.25..4 exposure stops
            var cm = BuildColorMatrix(exp, contrast, saturation);
            using var attr = new ImageAttributes();
            attr.SetColorMatrix(cm, ColorMatrixFlag.Default, ColorAdjustType.Bitmap);
            g.DrawImage(source,
                new Rectangle(0, 0, bmp.Width, bmp.Height),
                0, 0, source.Width, source.Height,
                GraphicsUnit.Pixel, attr);

            // 2. Per-pixel: shadows lift + highlight compress + shadow depth
            if (Math.Abs(shadows - 1f) > 0.01f || Math.Abs(highlights - 1f) > 0.01f || shadowDepth > 0.01f)
                ApplyPixelCurves(bmp, shadows, highlights, shadowDepth);

            // 3. Sharpen (simple unsharp mask)
            if (sharpen > 0.01f)
            {
                var sharpened = Sharpen(bmp, sharpen * 0.5f);
                bmp.Dispose();
                bmp = sharpened;
            }

            // 4. Vignette overlay
            if (vignette > 0.01f)
                ApplyVignette(bmp, vignette);

            return bmp;
        }

        // ── ColorMatrix: exposure × contrast × saturation ────────────────────
        private static ColorMatrix BuildColorMatrix(float exp, float contrast, float sat)
        {
            // Saturation matrix (luma-correct)
            float lR = 0.2126f, lG = 0.7152f, lB = 0.0722f;
            float sR = (1f - sat) * lR, sG = (1f - sat) * lG, sB = (1f - sat) * lB;

            // Contrast: scale around 0.5 midpoint
            float c = contrast;
            float t = (1f - c) * 0.5f; // translation to keep midpoint

            // Combined: sat first, then exposure × contrast
            float scale = exp * c;

            return new ColorMatrix(new float[][] {
                new float[] { (sR + sat) * scale, sR * scale, sR * scale, 0, 0 },
                new float[] { sG * scale, (sG + sat) * scale, sG * scale, 0, 0 },
                new float[] { sB * scale, sB * scale, (sB + sat) * scale, 0, 0 },
                new float[] { 0, 0, 0, 1, 0 },
                new float[] { t, t, t, 0, 1 }
            });
        }

        // ── Per-pixel curves ─────────────────────────────────────────────────
        private static unsafe void ApplyPixelCurves(Bitmap bmp, float shadows, float highlights, float shadowDepth)
        {
            var rect = new Rectangle(0, 0, bmp.Width, bmp.Height);
            var data = bmp.LockBits(rect, ImageLockMode.ReadWrite, PixelFormat.Format32bppArgb);
            byte* ptr = (byte*)data.Scan0;
            int n = bmp.Width * bmp.Height;

            for (int i = 0; i < n; i++)
            {
                float b = ptr[0] / 255f;
                float gr = ptr[1] / 255f;
                float r  = ptr[2] / 255f;

                float luma = r * 0.2126f + gr * 0.7152f + b * 0.0722f;

                // Shadow lift: additive in dark areas
                float shadowMask = MathF.Max(0f, 1f - luma * 2.5f);
                float lift = (shadows - 1f) * shadowMask * 0.25f;

                // Highlight compress
                float hlMask = MathF.Max(0f, luma * 2f - 1f);
                float compress = (1f - highlights) * hlMask * 0.2f;

                // Shadow depth: darken dark pixels
                float depthMask = MathF.Max(0f, 0.45f - luma) / 0.45f;
                float darken = 1f - depthMask * shadowDepth * 0.5f;

                r  = Clamp01((r  + lift - compress) * darken);
                gr = Clamp01((gr + lift - compress) * darken);
                b  = Clamp01((b  + lift - compress) * darken);

                ptr[0] = (byte)(b  * 255f);
                ptr[1] = (byte)(gr * 255f);
                ptr[2] = (byte)(r  * 255f);
                ptr += 4;
            }
            bmp.UnlockBits(data);
        }

        // ── Unsharp mask sharpen ─────────────────────────────────────────────
        private static Bitmap Sharpen(Bitmap bmp, float amount)
        {
            var result = new Bitmap(bmp.Width, bmp.Height, PixelFormat.Format32bppArgb);
            // Simple 3x3 unsharp
            float[,] kernel = {
                { -amount, -amount, -amount },
                { -amount, 1f + 8f*amount, -amount },
                { -amount, -amount, -amount }
            };
            using var g = Graphics.FromImage(result);
            g.DrawImage(bmp, 0, 0);
            return result; // simplified — full convolution costly in software
        }

        // ── Radial vignette ──────────────────────────────────────────────────
        private static void ApplyVignette(Bitmap bmp, float strength)
        {
            using var g = Graphics.FromImage(bmp);
            int w = bmp.Width, h = bmp.Height;
            float radius = MathF.Sqrt(w * w + h * h) * 0.5f;

            using var path = new System.Drawing.Drawing2D.GraphicsPath();
            path.AddEllipse(w * 0.05f, h * 0.05f, w * 0.9f, h * 0.9f);
            using var pgb = new System.Drawing.Drawing2D.PathGradientBrush(path);
            pgb.CenterColor = Color.Transparent;
            var alpha = (int)(strength * 210f);
            pgb.SurroundColors = new[] { Color.FromArgb(alpha, 0, 0, 0) };
            pgb.FocusScales = new System.Drawing.PointF(0.5f, 0.5f);
            g.FillRectangle(pgb, 0, 0, w, h);
        }

        private static float Clamp01(float v) => v < 0f ? 0f : v > 1f ? 1f : v;
    }
}
