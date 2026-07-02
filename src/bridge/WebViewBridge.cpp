#include "WebViewBridge.h"
#include "BinaryData.h"
#include "../engine/Params.h"

using WBC = juce::WebBrowserComponent;

// stringToMidiNote port (packages/midi/src/utils.ts): "C4" = 60
static int noteStringToMidi (const juce::String& note)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 11; i >= 0; --i) // check sharps before naturals
    {
        const juce::String n (names[i]);
        if (note.startsWith (n) && (n.length() == 2 || ! note.substring (1).startsWith ("#")))
        {
            const int octave = note.substring (n.length()).getIntValue();
            return (octave + 1) * 12 + i;
        }
    }
    return 60;
}

static juce::String mimeForPath (const juce::String& path)
{
    const auto ext = path.fromLastOccurrenceOf (".", false, false).toLowerCase();
    if (ext == "html") return "text/html; charset=utf-8";
    if (ext == "js" || ext == "mjs") return "text/javascript; charset=utf-8";
    if (ext == "css") return "text/css; charset=utf-8";
    if (ext == "png") return "image/png";
    if (ext == "json" || ext == "fnt") return "application/json";
    if (ext == "wasm") return "application/wasm";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "xml") return "application/xml";
    return "application/octet-stream";
}

juce::var WebViewBridge::buildReadyInfo()
{
    auto& apvts = processor.parameters();
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("sampleRate", processor.currentSampleRate());
    obj->setProperty ("nativeRate", processor.chipNativeRate());
    obj->setProperty ("version", JucePlugin_VersionString);

    auto* patch = new juce::DynamicObject();
    auto num = [&apvts] (const juce::String& id) { return apvts.getRawParameterValue (id)->load(); };
    patch->setProperty ("algorithm", (int) num ("alg"));
    patch->setProperty ("feedback", (int) num ("fb"));
    patch->setProperty ("masterGain", num ("master_gain"));
    patch->setProperty ("pan", num ("pan"));
    patch->setProperty ("polyphony", num ("poly") > 0.5f);

    auto* lfo = new juce::DynamicObject();
    lfo->setProperty ("freq", (int) num ("lfo_freq"));
    lfo->setProperty ("ams", (int) num ("lfo_ams"));
    lfo->setProperty ("pms", (int) num ("lfo_pms"));
    patch->setProperty ("lfo", juce::var (lfo));

    juce::Array<juce::var> ops;
    static const char* fields[] = { "ar", "dr", "sr", "rr", "sl", "tl", "ks", "mul", "dt", "am", "ssgeg" };
    static const char* jsNames[] = { "ar", "dr", "sr", "rr", "sl", "tl", "ks", "mul", "dt", "am", "ssgEg" };
    for (int op = 0; op < 4; ++op)
    {
        auto* o = new juce::DynamicObject();
        for (int f = 0; f < 11; ++f)
            o->setProperty (jsNames[f], (int) num (params::opId (op, fields[f])));
        ops.add (juce::var (o));
    }
    patch->setProperty ("operators", ops);
    obj->setProperty ("patch", juce::var (patch));
    return juce::var (obj);
}

void WebViewBridge::setParamFromUi (const juce::String& id, float value)
{
    if (auto* param = processor.parameters().getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (value));
        param->endChangeGesture();
    }
}

void WebViewBridge::handlePost (const juce::var& msg)
{
    const auto type = msg.getProperty ("type", {}).toString();

    if (type == "write")
    {
        processor.commands().push ({ EngineCommand::writeReg,
                                     (int) msg.getProperty ("port", 0),
                                     (int) msg.getProperty ("addr", 0),
                                     (int) msg.getProperty ("data", 0) });
    }
    else if (type == "reset")
    {
        processor.commands().push ({ EngineCommand::resetChip, 0, 0, 0 });
    }
    // Phase 4: scheduleEvents / clearEvents / startPlayback / stopPlayback / loadAdpcmB
}

juce::WebBrowserComponent::Options WebViewBridge::makeOptions()
{
    return WBC::Options()
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const juce::String& path) -> std::optional<WBC::Resource>
        {
            const juce::ScopedLock sl (zipLock);
            if (uiZip == nullptr)
                uiZip = std::make_unique<juce::ZipFile> (
                    new juce::MemoryInputStream (BinaryData::webui_zip, (size_t) BinaryData::webui_zipSize, false), true);

            auto entryPath = path == "/" ? juce::String ("index.html") : path.fromFirstOccurrenceOf ("/", false, false);
            const int index = uiZip->getIndexOfFileName (entryPath);
            if (index < 0)
                return std::nullopt;

            std::unique_ptr<juce::InputStream> stream (uiZip->createStreamForEntry (index));
            if (stream == nullptr)
                return std::nullopt;

            juce::MemoryBlock data;
            stream->readIntoMemoryBlock (data);
            WBC::Resource res;
            res.data.resize (data.getSize());
            std::memcpy (res.data.data(), data.getData(), data.getSize());
            res.mimeType = mimeForPath (entryPath);
            return res;
        })
        .withNativeFunction ("synthReady", [this] (const juce::Array<juce::var>&, WBC::NativeFunctionCompletion complete)
        {
            complete (buildReadyInfo());
        })
        .withNativeFunction ("synthNoteOn", [this] (const juce::Array<juce::var>& args, WBC::NativeFunctionCompletion complete)
        {
            if (args.size() > 0)
                processor.commands().push ({ EngineCommand::noteOn, noteStringToMidi (args[0].toString()), 0, 0 });
            complete ({});
        })
        .withNativeFunction ("synthNoteOff", [this] (const juce::Array<juce::var>& args, WBC::NativeFunctionCompletion complete)
        {
            if (args.size() > 0)
                processor.commands().push ({ EngineCommand::noteOff, noteStringToMidi (args[0].toString()), 0, 0 });
            complete ({});
        })
        .withNativeFunction ("synthAllNotesOff", [this] (const juce::Array<juce::var>&, WBC::NativeFunctionCompletion complete)
        {
            processor.commands().push ({ EngineCommand::allNotesOff, 0, 0, 0 });
            complete ({});
        })
        .withNativeFunction ("synthSetParam", [this] (const juce::Array<juce::var>& args, WBC::NativeFunctionCompletion complete)
        {
            if (args.size() >= 2)
                setParamFromUi (args[0].toString(), (float) (double) args[1]);
            complete ({});
        })
        .withNativeFunction ("synthSetOpEnvelope", [this] (const juce::Array<juce::var>& args, WBC::NativeFunctionCompletion complete)
        {
            // One IPC call per knob-drag event instead of 11 synthSetParam calls
            if (args.size() >= 2)
            {
                const int op = juce::jlimit (0, 3, (int) args[0]);
                const auto& env = args[1];
                static const std::pair<const char*, const char*> fields[] = {
                    { "ar", "ar" }, { "dr", "dr" }, { "sr", "sr" }, { "rr", "rr" },
                    { "sl", "sl" }, { "tl", "tl" }, { "ks", "ks" }, { "mul", "mul" },
                    { "dt", "dt" }, { "am", "am" }, { "ssgEg", "ssgeg" },
                };
                for (const auto& [jsName, paramName] : fields)
                {
                    const auto v = env.getProperty (jsName, juce::var());
                    if (! v.isVoid())
                        setParamFromUi (params::opId (op, paramName), (float) (double) v);
                }
            }
            complete ({});
        })
        .withNativeFunction ("synthPost", [this] (const juce::Array<juce::var>& args, WBC::NativeFunctionCompletion complete)
        {
            if (args.size() > 0)
                handlePost (args[0].isString() ? juce::JSON::parse (args[0].toString()) : args[0]);
            complete ({});
        });
}

void WebViewBridge::emitTimerEvents (juce::WebBrowserComponent& webView)
{
    // Host-MIDI notes → key highlight / Hibiki animation
    processor.noteEventsForUi().drain ([&webView] (const EngineCommand& cmd)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("note", juce::String (names[cmd.a % 12]) + juce::String (cmd.a / 12 - 1));
        webView.emitEventIfBrowserIsVisible (cmd.type == EngineCommand::noteOn ? "noteOn" : "noteOff",
                                             juce::var (obj));
    });

    // Oscilloscope waveform (AnalyserNode replacement). Standard base64 —
    // MemoryBlock::toBase64Encoding is a JUCE-specific alphabet that atob() can't read.
    uint8_t bytes[WaveformTap::kWindow];
    processor.waveform().readBytes (bytes);
    webView.emitEventIfBrowserIsVisible ("waveform", juce::var (juce::Base64::toBase64 (bytes, sizeof (bytes))));
}

void WebViewBridge::emitParamChanged (juce::WebBrowserComponent& webView, const juce::String& id, float value)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("id", id);
    obj->setProperty ("value", value);
    webView.emitEventIfBrowserIsVisible ("paramChanged", juce::var (obj));
}
