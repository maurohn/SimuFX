using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace SimuConfig
{
    /// <summary>Minimal INI read/write that handles gMotor2's comment-heavy format.</summary>
    public class IniFile
    {
        // section → key → value (preserves insertion order)
        private readonly Dictionary<string, Dictionary<string, string>> _data =
            new(StringComparer.OrdinalIgnoreCase);
        private string _rawHeader = ""; // preserve leading comment lines

        public void Load(string path)
        {
            _data.Clear();
            _rawHeader = "";
            string? section = null;
            bool headerDone = false;
            var sb = new StringBuilder();

            foreach (var line in File.ReadAllLines(path, Encoding.UTF8))
            {
                var t = line.Trim();
                if (!headerDone && (t.StartsWith("//") || t.StartsWith(";") || t.Length == 0))
                { sb.AppendLine(line); continue; }
                headerDone = true;

                if (t.StartsWith("[") && t.EndsWith("]"))
                {
                    section = t[1..^1];
                    if (!_data.ContainsKey(section))
                        _data[section] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                }
                else if (section != null && t.Contains('=') && !t.StartsWith("//") && !t.StartsWith(";"))
                {
                    var idx = t.IndexOf('=');
                    var key = t[..idx].Trim();
                    var val = t[(idx + 1)..].Trim();
                    _data[section][key] = val;
                }
            }
            _rawHeader = sb.ToString();
        }

        public void Save(string path)
        {
            var sb = new StringBuilder();
            if (!string.IsNullOrEmpty(_rawHeader)) sb.Append(_rawHeader);
            foreach (var sec in _data)
            {
                sb.AppendLine($"[{sec.Key}]");
                foreach (var kv in sec.Value)
                    sb.AppendLine($"{kv.Key}={kv.Value}");
                sb.AppendLine();
            }
            File.WriteAllText(path, sb.ToString(), Encoding.UTF8);
        }

        public string Get(string section, string key, string def = "")
        {
            if (_data.TryGetValue(section, out var sec) && sec.TryGetValue(key, out var v))
                return v;
            return def;
        }

        public bool GetBool(string section, string key, bool def) =>
            Get(section, key, def ? "true" : "false") is "true" or "1" or "yes";

        public float GetFloat(string section, string key, float def) =>
            float.TryParse(Get(section, key, def.ToString("F2")),
                System.Globalization.NumberStyles.Float,
                System.Globalization.CultureInfo.InvariantCulture, out float v) ? v : def;

        public void Set(string section, string key, string value)
        {
            if (!_data.ContainsKey(section))
                _data[section] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            _data[section][key] = value;
        }

        /// <summary>Overlay another INI on top of this one (for preset merging).</summary>
        public void MergeFrom(IniFile other)
        {
            foreach (var sec in other._data)
                foreach (var kv in sec.Value)
                    Set(sec.Key, kv.Key, kv.Value);
        }
    }
}
