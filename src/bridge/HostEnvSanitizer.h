#pragma once

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

// Hosts that ship bundled gtk/glib (Ardour, some Bitwig/Waveform builds)
// export LD_LIBRARY_PATH and friends pointing into their bundle. The JUCE
// webview helper is forked during WebBrowserComponent construction and
// inherits that environment, so the system webkit2gtk loads against the
// host's incompatible libs and dies — leaving a white editor.
// Scope this around WebBrowserComponent construction: strips the poisoned
// variables for the fork, restores them immediately afterwards so the host
// process keeps its own environment intact.
class ScopedHostEnvSanitizer
{
public:
    ScopedHostEnvSanitizer()
    {
        static const char* keys[] = {
            "LD_LIBRARY_PATH",
            "GTK_PATH",
            "GTK_MODULES",
            "GTK_DATA_PREFIX",
            "GTK_EXE_PREFIX",
            "GTK_IM_MODULE_FILE",
            "GIO_MODULE_DIR",
            "GDK_PIXBUF_MODULE_FILE",
            "GDK_PIXBUF_MODULEDIR",
            "FONTCONFIG_FILE",
            "FONTCONFIG_PATH",
            "PANGO_RC_FILE",
            "PANGO_LIBDIR",
            "PANGO_SYSCONFDIR",
        };
        for (const char* key : keys)
            if (const char* value = std::getenv (key))
            {
                saved.emplace_back (key, value);
                ::unsetenv (key);
            }
    }

    ~ScopedHostEnvSanitizer()
    {
        for (const auto& [key, value] : saved)
            ::setenv (key.c_str(), value.c_str(), 1);
    }

private:
    std::vector<std::pair<std::string, std::string>> saved;
};
