using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace SimuConfig
{
    /// <summary>
    /// INI reader/writer that preserves the original file structure.
    /// Only modifies lines whose keys were Set(). Everything else stays intact.
    /// </summary>
    public class IniFile
    {
        private readonly Dictionary<string, Dictionary<string, string>> _data =
            new(StringComparer.OrdinalIgnoreCase);

        // Store original lines to preserve file format on save
        private readonly List<string> _originalLines = new();
        private string _sourcePath = "";

        public void Load(string path)
        {
            _data.Clear();
            _originalLines.Clear();
            _sourcePath = path;
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

        /// <summary>
        /// Save by modifying only changed keys in-place, preserving all other lines.
        /// New keys/sections are appended at the end.
        /// </summary>
        public void Save(string path)
        {
            var modified = new Dictionary<string, HashSet<string>>(StringComparer.OrdinalIgnoreCase);
            foreach (var sec in _dirty)
            {
                if (!modified.ContainsKey(sec.Key))
                    modified[sec.Key] = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                foreach (var key in sec.Value)
                    modified[sec.Key].Add(key);
            }

            // If we have original lines, do in-place replacement
            if (_originalLines.Count > 0 && File.Exists(path))
            {
                var sb = new StringBuilder();
                string? currentSection = null;
                var writtenKeys = new Dictionary<string, HashSet<string>>(StringComparer.OrdinalIgnoreCase);

                foreach (var line in _originalLines)
                {
                    var t = line.Trim();

                    if (t.StartsWith("[") && t.EndsWith("]"))
                    {
                        currentSection = t[1..^1];
                        sb.AppendLine(line);
                    }
                    else if (currentSection != null && t.Contains('=') && !t.StartsWith("//") && !t.StartsWith(";"))
                    {
                        var idx = t.IndexOf('=');
                        var key = t[..idx].Trim();

                        // If this key was modified, write new value
                        if (_data.TryGetValue(currentSection, out var sec) && sec.TryGetValue(key, out var val))
                        {
                            sb.AppendLine($"{key}={val}");
                            if (!writtenKeys.ContainsKey(currentSection))
                                writtenKeys[currentSection] = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                            writtenKeys[currentSection].Add(key);
                        }
                        else
                        {
                            sb.AppendLine(line); // preserve original
                        }
                    }
                    else
                    {
                        sb.AppendLine(line); // preserve comments, empty lines, etc.
                    }
                }

                // Append any NEW keys that weren't in the original file
                foreach (var sec in _dirty)
                {
                    foreach (var key in sec.Value)
                    {
                        if (writtenKeys.TryGetValue(sec.Key, out var wk) && wk.Contains(key))
                            continue; // already written
                        // Need to add this key — find or create section
                        if (!writtenKeys.ContainsKey(sec.Key))
                        {
                            sb.AppendLine($"\n[{sec.Key}]");
                            writtenKeys[sec.Key] = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                        }
                        sb.AppendLine($"{key}={_data[sec.Key][key]}");
                        writtenKeys[sec.Key].Add(key);
                    }
                }

                File.WriteAllText(path, sb.ToString(), Encoding.UTF8);
            }
            else
            {
                // No original lines — write fresh
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

            _dirty.Clear();
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

        // Track which keys were explicitly Set() so we know what to write
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

        /// <summary>Overlay another INI on top of this one (for preset merging).</summary>
        public void MergeFrom(IniFile other)
        {
            foreach (var sec in other._data)
                foreach (var kv in sec.Value)
                    Set(sec.Key, kv.Key, kv.Value);
        }
    }
}
