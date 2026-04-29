using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace SimuConfig
{
    /// <summary>
    /// INI reader/writer that preserves the original file structure.
    /// Modified keys are updated in-place; new keys are inserted into their section.
    /// </summary>
    public class IniFile
    {
        private readonly Dictionary<string, Dictionary<string, string>> _data =
            new(StringComparer.OrdinalIgnoreCase);

        private readonly List<string> _originalLines = new();

        public void Load(string path)
        {
            _data.Clear();
            _originalLines.Clear();
            _dirty.Clear();
            string? section = null;

            foreach (var line in File.ReadAllLines(path, Encoding.UTF8))
            {
                _originalLines.Add(line);
                var t = line.Trim();

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
        }

        public void Save(string path)
        {
            if (_originalLines.Count > 0 && File.Exists(path))
                SaveInPlace(path);
            else
                SaveFresh(path);

            _dirty.Clear();
        }

        private void SaveInPlace(string path)
        {
            var sb = new StringBuilder();
            string? currentSection = null;
            string? nextSection = null;
            var writtenKeys = new Dictionary<string, HashSet<string>>(StringComparer.OrdinalIgnoreCase);

            for (int i = 0; i < _originalLines.Count; i++)
            {
                var line = _originalLines[i];
                var t = line.Trim();

                if (t.StartsWith("[") && t.EndsWith("]"))
                {
                    // Before moving to next section, flush any NEW dirty keys for the current section
                    if (currentSection != null)
                        FlushNewKeys(sb, currentSection, writtenKeys);

                    currentSection = t[1..^1];
                    if (!writtenKeys.ContainsKey(currentSection))
                        writtenKeys[currentSection] = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                    sb.AppendLine(line);
                }
                else if (currentSection != null && t.Contains('=') && !t.StartsWith("//") && !t.StartsWith(";"))
                {
                    var idx = t.IndexOf('=');
                    var key = t[..idx].Trim();

                    // Write updated value if dirty, otherwise preserve original
                    if (_dirty.TryGetValue(currentSection, out var dirtyKeys) && dirtyKeys.Contains(key))
                    {
                        sb.AppendLine($"{key}={_data[currentSection][key]}");
                    }
                    else if (_data.TryGetValue(currentSection, out var sec) && sec.TryGetValue(key, out var val))
                    {
                        sb.AppendLine($"{key}={val}");
                    }
                    else
                    {
                        sb.AppendLine(line);
                    }

                    if (!writtenKeys.ContainsKey(currentSection))
                        writtenKeys[currentSection] = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                    writtenKeys[currentSection].Add(key);
                }
                else
                {
                    sb.AppendLine(line);
                }
            }

            // Flush remaining new keys for the last section
            if (currentSection != null)
                FlushNewKeys(sb, currentSection, writtenKeys);

            // Append entirely new sections
            foreach (var sec in _dirty)
            {
                if (writtenKeys.ContainsKey(sec.Key)) continue; // section already processed
                sb.AppendLine();
                sb.AppendLine($"[{sec.Key}]");
                writtenKeys[sec.Key] = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                foreach (var key in sec.Value)
                {
                    sb.AppendLine($"{key}={_data[sec.Key][key]}");
                    writtenKeys[sec.Key].Add(key);
                }
            }

            File.WriteAllText(path, sb.ToString(), Encoding.UTF8);
        }

        /// <summary>Insert any dirty keys that weren't already written for this section.</summary>
        private void FlushNewKeys(StringBuilder sb, string section,
            Dictionary<string, HashSet<string>> writtenKeys)
        {
            if (!_dirty.TryGetValue(section, out var dirtyKeys)) return;
            if (!writtenKeys.TryGetValue(section, out var written))
            {
                written = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                writtenKeys[section] = written;
            }

            foreach (var key in dirtyKeys)
            {
                if (written.Contains(key)) continue;
                sb.AppendLine($"{key}={_data[section][key]}");
                written.Add(key);
            }
        }

        private void SaveFresh(string path)
        {
            var sb = new StringBuilder();
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
            float.TryParse(Get(section, key, def.ToString("F2",
                System.Globalization.CultureInfo.InvariantCulture)),
                System.Globalization.NumberStyles.Float,
                System.Globalization.CultureInfo.InvariantCulture, out float v) ? v : def;

        private readonly Dictionary<string, HashSet<string>> _dirty =
            new(StringComparer.OrdinalIgnoreCase);

        public void Set(string section, string key, string value)
        {
            if (!_data.ContainsKey(section))
                _data[section] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            _data[section][key] = value;

            if (!_dirty.ContainsKey(section))
                _dirty[section] = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            _dirty[section].Add(key);
        }

        public void MergeFrom(IniFile other)
        {
            foreach (var sec in other._data)
                foreach (var kv in sec.Value)
                    Set(sec.Key, kv.Key, kv.Value);
        }
    }
}
