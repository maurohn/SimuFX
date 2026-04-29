using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Forms;
using System.Windows.Media.Imaging;

namespace SimuConfig
{
    public partial class MainWindow : Window
    {
        private string _gamePath = "";
        private string _simufxPath = "";
        private bool _loading = true;
        private System.Threading.Timer? _previewTimer;
        private System.Drawing.Bitmap? _sourceImage;
        private readonly IniFile _gameIni = new();
        private readonly IniFile _fxIni = new();

        // ── Resolution list built from EnumDisplaySettings ──────────────────
        private static readonly (int w, int h)[] StdRes = {
            (800,600),(1024,768),(1280,720),(1280,768),(1280,1024),
            (1366,768),(1440,900),(1600,900),(1680,1050),(1920,1080),
            (1920,1200),(2560,1440),(3840,2160)
        };

        // ── FSAA tag → Config.ini value ──────────────────────────────────────
        private static readonly Dictionary<int, int> FsaaMap = new()
        { {0,0},{1,1},{2,3},{3,7},{4,15},{5,23},{6,31},{7,39} };

        public MainWindow()
        {
            InitializeComponent();
            Loaded += OnLoaded;
        }

        // ────────────────────────────────────────────────────────────────────
        private void OnLoaded(object s, RoutedEventArgs e)
        {
            PopulateResolutions();
            DetectGPU();
            AutoDetectGamePath();
            LoadPreviewImage();
            _loading = false;
        }

        // ── Game path detection ──────────────────────────────────────────────
        private void AutoDetectGamePath()
        {
            var candidates = new[] {
                @"D:\SteamLibrary\steamapps\common\SimuV3",
                @"C:\SteamLibrary\steamapps\common\SimuV3",
                Path.Combine(AppDomain.CurrentDomain.BaseDirectory)
            };
            foreach (var c in candidates)
                if (File.Exists(Path.Combine(c, "Config.ini"))) { SetGamePath(c); return; }

            GamePathLabel.Text = "Ruta del juego: no encontrado";
            StatusText.Text = "⚠  No se encontró el juego. Usá 📁 para seleccionar la carpeta.";
        }

        private void SetGamePath(string path)
        {
            _gamePath   = path;
            _simufxPath = Path.Combine(path, "SimuFX");
            GamePathLabel.Text = $"Ruta: {path}";
            SubtitleText.Text  = $"SimuV3 — {path}";
            LoadGameIni();
            LoadSimuFXIni();
            ScanPresets();
            StatusText.Text = "Configuración cargada.";
        }

        private void BrowseGamePath_Click(object s, RoutedEventArgs e)
        {
            using var dlg = new FolderBrowserDialog { Description = "Seleccioná la carpeta del juego" };
            if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                SetGamePath(dlg.SelectedPath);
        }

        // ── INI loading ──────────────────────────────────────────────────────
        private void LoadGameIni()
        {
            _loading = true;
            var f = Path.Combine(_gamePath, "Config.ini");
            if (!File.Exists(f)) { _loading = false; return; }
            _gameIni.Load(f);

            // Adapter
            var driver = _gameIni.Get("COMPONENTS", "VideoDriver", "0");
            // Resolution: try CustomVidRes first, then VideoMode as fallback
            var customRes = _gameIni.Get("COMPONENTS", "CustomVidRes", "(0, 0)");
            if (customRes != "(0, 0)" && TryParseCustomRes(customRes, out int cw, out int ch))
                SelectResolutionByWH(cw, ch);
            else
            {
                var mode = _gameIni.Get("COMPONENTS", "VideoMode", "10");
                SelectResolutionByMode(int.TryParse(mode, out int m) ? m : 10);
            }

            // FSAA
            var fsaa = int.TryParse(_gameIni.Get("COMPONENTS","FSAA","0"), out int f2) ? f2 : 0;
            SelectFsaa(fsaa);

            // ShaderLevel
            var sl = int.TryParse(_gameIni.Get("COMPONENTS","ShaderLevel","3"), out int s2) ? s2 : 3;
            CbShaderLevel.SelectedIndex = Math.Clamp(s2, 0, 3);

            // Refresh
            var ref_ = int.TryParse(_gameIni.Get("COMPONENTS","VideoRefresh","0"), out int r) ? r : 0;
            SelectRefresh(ref_);

            // Booleans
            ChkVSync.IsChecked     = _gameIni.Get("COMPONENTS","VSync","1") == "1";
            ChkWidescreen.IsChecked= _gameIni.Get("COMPONENTS","WidescreenUI","1") == "1";
            ChkWindowed.IsChecked  = _gameIni.Get("COMPONENTS","WindowedMode","1") == "1";
            ChkMultiView.IsChecked = _gameIni.Get("COMPONENTS","SubViews","0") == "1";

            // Language
            var lang = _gameIni.Get("COMPONENTS","LanguagesFile","SPANISH.DIC").ToUpper();
            foreach (System.Windows.Controls.ComboBoxItem item in CbLanguage.Items)
                if (item.Tag?.ToString()?.ToUpper() == lang) { CbLanguage.SelectedItem = item; break; }

            _loading = false;
        }

        private void LoadSimuFXIni()
        {
            var f = Path.Combine(_simufxPath, "global.ini");
            if (!File.Exists(f)) return;
            _fxIni.Load(f);
            ApplyFXToControls();
        }

        /// <summary>Apply current _fxIni values to all SimuFX controls without reloading from disk.</summary>
        private void ApplyFXToControls()
        {
            _loading = true;

            ChkTonemap.IsChecked    = _fxIni.GetBool("Tonemap","Enabled", true);
            SldExposure.Value       = _fxIni.GetFloat("Tonemap","Exposure", 0.08f);
            SldContrast.Value       = _fxIni.GetFloat("Tonemap","Contrast", 1.14f);
            SldShadows.Value        = _fxIni.GetFloat("Tonemap","Shadows", 1.25f);
            SldHighlights.Value     = _fxIni.GetFloat("Tonemap","Highlights", 0.88f);

            ChkColor.IsChecked      = _fxIni.GetBool("Color","Enabled", true);
            SldSaturation.Value     = _fxIni.GetFloat("Color","Saturation", 1.2f);
            SldVibrance.Value       = _fxIni.GetFloat("Color","Vibrance", 0.22f);

            ChkFXAA.IsChecked       = _fxIni.GetBool("AntiAliasing","Enabled", true);
            SldFXAA.Value           = _fxIni.GetFloat("AntiAliasing","Strength", 1.0f);

            ChkSharpen.IsChecked    = _fxIni.GetBool("Sharpen","Enabled", true);
            SldSharpen.Value        = _fxIni.GetFloat("Sharpen","Strength", 0.32f);

            ChkAO.IsChecked         = _fxIni.GetBool("AmbientOcclusion","Enabled", true);
            SldAO.Value             = _fxIni.GetFloat("AmbientOcclusion","Strength", 0.60f);

            ChkShadowDepth.IsChecked= _fxIni.GetBool("ShadowDepth","Enabled", true);
            SldShadowDepth.Value    = _fxIni.GetFloat("ShadowDepth","Depth", 0.50f);

            ChkVignette.IsChecked   = _fxIni.GetBool("Vignette","Enabled", false);
            SldVignette.Value       = _fxIni.GetFloat("Vignette","Intensity", 0.10f);

            _loading = false;
            UpdateLabels();
            SchedulePreviewUpdate();
        }

        // ── Presets ──────────────────────────────────────────────────────────
        private void ScanPresets()
        {
            CbPreset.Items.Clear();
            var dir = Path.Combine(_simufxPath, "presets");
            if (!Directory.Exists(dir)) return;
            foreach (var f in Directory.GetFiles(dir,"*.ini").OrderBy(x => x))
                CbPreset.Items.Add(Path.GetFileNameWithoutExtension(f));
            // Select current
            var cur = _fxIni.Get("General","Preset","Dynamic");
            if (CbPreset.Items.Contains(cur)) CbPreset.SelectedItem = cur;
        }

        private void CbPreset_SelectionChanged(object s, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (_loading || CbPreset.SelectedItem == null) return;
            var name = CbPreset.SelectedItem.ToString()!;
            var file = Path.Combine(_simufxPath, "presets", name + ".ini");
            if (!File.Exists(file)) return;
            var preset = new IniFile();
            preset.Load(file);
            _fxIni.MergeFrom(preset);
            ApplyFXToControls(); // Don't reload from disk — use merged values
            StatusText.Text = $"Preset '{name}' aplicado.";
        }

        private void LoadPreset_Click(object s, RoutedEventArgs e)
        {
            if (CbPreset.SelectedItem == null) return;
            var name = CbPreset.SelectedItem.ToString()!;
            var preset = new IniFile();
            preset.Load(Path.Combine(_simufxPath, "presets", name + ".ini"));
            _fxIni.MergeFrom(preset);
            ApplyFXToControls();
            StatusText.Text = $"Preset '{name}' cargado.";
        }

        private void SavePreset_Click(object s, RoutedEventArgs e)
        {
            var name = TxtNewPreset.Text.Trim();
            if (string.IsNullOrEmpty(name)) { StatusText.Text = "⚠  Ingresá un nombre para el preset."; return; }
            var path = Path.Combine(_simufxPath, "presets", name + ".ini");
            SaveSimuFXIni(path);
            ScanPresets();
            CbPreset.SelectedItem = name;
            TxtNewPreset.Text = "";
            StatusText.Text = $"Preset '{name}' guardado.";
        }

        // ── Resolution helpers ────────────────────────────────────────────────
        private void PopulateResolutions()
        {
            CbResolution.Items.Clear();
            CbRefresh.Items.Clear();

            var added = new HashSet<string>();
            var refreshRates = new SortedSet<int>();

            try
            {
                // Use CIM_VideoControllerResolution — same modes DirectX enumerates
                var searcher = new System.Management.ManagementObjectSearcher(
                    "select * from CIM_VideoControllerResolution");
                foreach (System.Management.ManagementObject o in searcher.Get())
                {
                    var w = Convert.ToInt32(o["HorizontalResolution"]);
                    var h = Convert.ToInt32(o["VerticalResolution"]);
                    var colors = Convert.ToUInt64(o["NumberOfColors"]);
                    var hz = Convert.ToInt32(o["RefreshRate"]);

                    // Only 32-bit modes (NumberOfColors = 4294967296 = 2^32)
                    if (colors < 65536 || w < 640) continue;

                    var key = $"{w}x{h}";
                    if (added.Add(key))
                        CbResolution.Items.Add(new System.Windows.Controls.ComboBoxItem
                            { Content = $"{w} x {h}", Tag = $"{w},{h}" });

                    if (hz > 0) refreshRates.Add(hz);
                }
            }
            catch
            {
                // Fallback: add standard resolutions
                foreach (var (w, h) in StdRes)
                    CbResolution.Items.Add(new System.Windows.Controls.ComboBoxItem
                        { Content = $"{w} x {h}", Tag = $"{w},{h}" });
                refreshRates.Add(60);
            }

            if (CbResolution.Items.Count > 0) CbResolution.SelectedIndex = 0;

            // Populate refresh rates from actual supported values
            foreach (var hz in refreshRates)
                CbRefresh.Items.Add(new System.Windows.Controls.ComboBoxItem
                    { Content = $"{hz} Hz", Tag = hz.ToString() });

            if (CbRefresh.Items.Count > 0) CbRefresh.SelectedIndex = 0;
        }

        private void SelectResolutionByMode(int mode)
        {
            if (CbResolution.Items.Count > mode && mode >= 0)
                CbResolution.SelectedIndex = mode;
        }

        private void SelectResolutionByWH(int w, int h)
        {
            var target = $"{w}x{h}";
            for (int i = 0; i < CbResolution.Items.Count; i++)
            {
                var item = CbResolution.Items[i] as System.Windows.Controls.ComboBoxItem;
                var tag = item?.Tag?.ToString()?.Replace(",","x").Replace(" ","");
                if (tag == target) { CbResolution.SelectedIndex = i; return; }
            }
        }

        private static bool TryParseCustomRes(string val, out int w, out int h)
        {
            w = h = 0;
            val = val.Trim('(', ')', ' ');
            var parts = val.Split(',');
            if (parts.Length != 2) return false;
            return int.TryParse(parts[0].Trim(), out w) && int.TryParse(parts[1].Trim(), out h) && w > 0;
        }

        private void SelectFsaa(int val)
        {
            var idx = FsaaMap.FirstOrDefault(kv => kv.Value == val).Key;
            CbFSAA.SelectedIndex = Math.Clamp(idx, 0, CbFSAA.Items.Count - 1);
        }

        private void SelectRefresh(int hz)
        {
            foreach (System.Windows.Controls.ComboBoxItem item in CbRefresh.Items)
                if (item.Tag?.ToString() == hz.ToString()) { CbRefresh.SelectedItem = item; return; }
            // Fallback: closest match
            if (CbRefresh.Items.Count > 0) CbRefresh.SelectedIndex = CbRefresh.Items.Count - 1;
        }

        // ── GPU info ─────────────────────────────────────────────────────────
        private void DetectGPU()
        {
            try {
                var s = new System.Management.ManagementObjectSearcher("select * from Win32_VideoController");
                foreach (System.Management.ManagementObject o in s.Get()) {
                    var name = o["Name"]?.ToString() ?? "Unknown";
                    var vram = (uint)(o["AdapterRAM"] ?? 0u);
                    var driver = o["DriverVersion"]?.ToString() ?? "—";
                    LblGPU.Text   = $"GPU: {name}";
                    LblVRAM.Text  = $"VRAM: {vram/1024/1024} MB";
                    LblDriver.Text= $"Driver: {driver}";
                    // Populate adapter combo
                    CbAdapter.Items.Add(name);
                    break;
                }
                if (CbAdapter.Items.Count > 0) CbAdapter.SelectedIndex = 0;
            } catch { LblGPU.Text = "GPU: no detectada"; }
        }

        // ── Preview ──────────────────────────────────────────────────────────
        private void LoadPreviewImage()
        {
            try {
                var uri = new Uri("pack://application:,,,/assets/preview.jpg");
                using var stream = System.Windows.Application.GetResourceStream(uri)?.Stream;
                if (stream == null) return;
                _sourceImage = new System.Drawing.Bitmap(stream);
                ApplyPreview();
            } catch { }
        }

        private void SchedulePreviewUpdate()
        {
            ProcessingBadge.Visibility = Visibility.Visible;
            _previewTimer?.Dispose();
            _previewTimer = new System.Threading.Timer(_ =>
                Dispatcher.Invoke(() => { ApplyPreview(); ProcessingBadge.Visibility = Visibility.Collapsed; }),
                null, 120, System.Threading.Timeout.Infinite);
        }

        private void ApplyPreview()
        {
            if (_sourceImage == null) return;
            try {
                var bmp = PreviewProcessor.Apply(_sourceImage,
                    exposure:    (float)SldExposure.Value,
                    contrast:    (float)SldContrast.Value,
                    saturation:  (float)SldSaturation.Value,
                    shadows:     (float)SldShadows.Value,
                    highlights:  (float)SldHighlights.Value,
                    sharpen:     ChkSharpen.IsChecked == true ? (float)SldSharpen.Value : 0f,
                    vignette:    ChkVignette.IsChecked == true ? (float)SldVignette.Value : 0f,
                    shadowDepth: ChkShadowDepth.IsChecked == true ? (float)SldShadowDepth.Value : 0f);

                PreviewImage.Source = BitmapToSource(bmp);
                bmp.Dispose();
            } catch { }
        }

        private static BitmapSource BitmapToSource(System.Drawing.Bitmap bmp)
        {
            var data = bmp.LockBits(new System.Drawing.Rectangle(0,0,bmp.Width,bmp.Height),
                System.Drawing.Imaging.ImageLockMode.ReadOnly,
                System.Drawing.Imaging.PixelFormat.Format32bppArgb);
            var src = System.Windows.Media.Imaging.BitmapSource.Create(
                bmp.Width, bmp.Height, 96, 96,
                System.Windows.Media.PixelFormats.Bgra32, null,
                data.Scan0, data.Stride * bmp.Height, data.Stride);
            bmp.UnlockBits(data);
            src.Freeze();
            return src;
        }

        // ── Event handlers ────────────────────────────────────────────────────
        private void OnGameSettingChanged(object s, object e) { /* Saved on OK */ }

        private void OnFXSettingChanged(object s, object e)
        {
            if (_loading) return;
            UpdateLabels();
            SchedulePreviewUpdate();
        }

        private void UpdateLabels()
        {
            LblExposure.Text    = SldExposure.Value.ToString("F2");
            LblContrast.Text    = SldContrast.Value.ToString("F2");
            LblShadows.Text     = SldShadows.Value.ToString("F2");
            LblHighlights.Text  = SldHighlights.Value.ToString("F2");
            LblSaturation.Text  = SldSaturation.Value.ToString("F2");
            LblVibrance.Text    = SldVibrance.Value.ToString("F2");
            LblFXAA.Text        = SldFXAA.Value.ToString("F2");
            LblSharpen.Text     = SldSharpen.Value.ToString("F2");
            LblAO.Text          = SldAO.Value.ToString("F2");
            LblShadowDepth.Text = SldShadowDepth.Value.ToString("F2");
            LblVignette.Text    = SldVignette.Value.ToString("F2");
        }

        // ── Save ──────────────────────────────────────────────────────────────
        private void SaveAll_Click(object s, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(_gamePath))
            { StatusText.Text = "⚠  No hay ruta de juego configurada."; return; }
            SaveGameIni();
            SaveSimuFXIni(Path.Combine(_simufxPath, "global.ini"));
            StatusText.Text = "✓  Guardado correctamente.";
            System.Windows.MessageBox.Show("Configuración guardada.\nReiniciá el juego para aplicar los cambios.",
                "SimuConfig", MessageBoxButton.OK, MessageBoxImage.Information);
            Close();
        }

        private void Cancel_Click(object s, RoutedEventArgs e) => Close();

        private void SaveGameIni()
        {
            var f = Path.Combine(_gamePath, "Config.ini");
            if (!File.Exists(f)) return;
            _gameIni.Load(f);

            _gameIni.Set("COMPONENTS","VideoDriver",   CbAdapter.SelectedIndex.ToString());

            // Save resolution as CustomVidRes for reliable persistence
            var resItem = CbResolution.SelectedItem as System.Windows.Controls.ComboBoxItem;
            if (resItem?.Tag != null)
            {
                var parts = resItem.Tag.ToString()!.Split(',');
                if (parts.Length == 2)
                    _gameIni.Set("COMPONENTS","CustomVidRes", $"({parts[0]}, {parts[1]})");
            }

            _gameIni.Set("COMPONENTS","ShaderLevel",   CbShaderLevel.SelectedIndex.ToString());

            var fsaaIdx = CbFSAA.SelectedIndex;
            _gameIni.Set("COMPONENTS","FSAA", FsaaMap.TryGetValue(fsaaIdx, out int fv) ? fv.ToString() : "0");

            var refItem = CbRefresh.SelectedItem as System.Windows.Controls.ComboBoxItem;
            _gameIni.Set("COMPONENTS","VideoRefresh", refItem?.Tag?.ToString() ?? "0");

            _gameIni.Set("COMPONENTS","VSync",       ChkVSync.IsChecked == true ? "1":"0");
            _gameIni.Set("COMPONENTS","WidescreenUI",ChkWidescreen.IsChecked == true ? "1":"0");
            _gameIni.Set("COMPONENTS","WindowedMode",ChkWindowed.IsChecked == true ? "1":"0");
            _gameIni.Set("COMPONENTS","SubViews",    ChkMultiView.IsChecked == true ? "1":"0");

            var langItem = CbLanguage.SelectedItem as System.Windows.Controls.ComboBoxItem;
            if (langItem?.Tag != null)
                _gameIni.Set("COMPONENTS","LanguagesFile", langItem.Tag.ToString()!);

            _gameIni.Save(f);
        }

        private void SaveSimuFXIni(string path)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            var ini = new IniFile();
            if (File.Exists(path)) ini.Load(path);

            var ic = System.Globalization.CultureInfo.InvariantCulture;

            ini.Set("General","Preset", CbPreset.SelectedItem?.ToString() ?? "Dynamic");
            ini.Set("General","Enabled", "true");
            ini.Set("General","ShowOverlay", "true");
            ini.Set("General","UseDirectValues", "true");

            ini.Set("Tonemap","Enabled",    ChkTonemap.IsChecked == true ? "true":"false");
            ini.Set("Tonemap","Exposure",   SldExposure.Value.ToString("F2", ic));
            ini.Set("Tonemap","Contrast",   SldContrast.Value.ToString("F2", ic));
            ini.Set("Tonemap","Shadows",    SldShadows.Value.ToString("F2", ic));
            ini.Set("Tonemap","Highlights", SldHighlights.Value.ToString("F2", ic));

            ini.Set("Color","Enabled",    ChkColor.IsChecked == true ? "true":"false");
            ini.Set("Color","Saturation", SldSaturation.Value.ToString("F2", ic));
            ini.Set("Color","Vibrance",   SldVibrance.Value.ToString("F2", ic));

            ini.Set("AntiAliasing","Enabled",  ChkFXAA.IsChecked == true ? "true":"false");
            ini.Set("AntiAliasing","Strength", SldFXAA.Value.ToString("F2", ic));

            ini.Set("Sharpen","Enabled",   ChkSharpen.IsChecked == true ? "true":"false");
            ini.Set("Sharpen","Strength",  SldSharpen.Value.ToString("F2", ic));

            ini.Set("AmbientOcclusion","Enabled",  ChkAO.IsChecked == true ? "true":"false");
            ini.Set("AmbientOcclusion","Strength", SldAO.Value.ToString("F2", ic));

            ini.Set("ShadowDepth","Enabled", ChkShadowDepth.IsChecked == true ? "true":"false");
            ini.Set("ShadowDepth","Depth",   SldShadowDepth.Value.ToString("F2", ic));

            ini.Set("Vignette","Enabled",   ChkVignette.IsChecked == true ? "true":"false");
            ini.Set("Vignette","Intensity", SldVignette.Value.ToString("F2", ic));

            // Anisotropic level
            int[] anisoVals = { 1,2,4,8,16 };
            var aVal = anisoVals[Math.Clamp(CbAnisotropic.SelectedIndex, 0, 4)];
            ini.Set("Textures","Anisotropic", aVal.ToString());

            ini.Save(path);
        }
    }
}
