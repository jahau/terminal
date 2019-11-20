// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "AppHost.h"
#include "resource.h"

using namespace winrt;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt::Windows::Foundation::Numerics;

// Note: Generate GUID using TlgGuid.exe tool - seriously, it won't work if you
// just generate an arbitrary GUID
TRACELOGGING_DEFINE_PROVIDER(
    g_hWindowsTerminalProvider,
    "Microsoft.Windows.Terminal.Win32Host",
    // {56c06166-2e2e-5f4d-7ff3-74f4b78c87d6}
    (0x56c06166, 0x2e2e, 0x5f4d, 0x7f, 0xf3, 0x74, 0xf4, 0xb7, 0x8c, 0x87, 0xd6),
    TraceLoggingOptionMicrosoftTelemetry());

// Routine Description:
// - Retrieves the string resource from the current module with the given ID
//   from the resources files. See resource.h and the .rc definitions for valid IDs.
// Arguments:
// - id - Resource ID
// Return Value:
// - String resource retrieved from that ID.
static std::wstring GetStringResource(const UINT id)
{
    // Calling LoadStringW with a pointer-sized storage and no length will return a read-only pointer
    // directly to the resource data instead of copying it immediately into a buffer.
    LPWSTR readOnlyResource = nullptr;
    const auto length = LoadStringW(wil::GetModuleInstanceHandle(),
                                    id,
                                    reinterpret_cast<LPWSTR>(&readOnlyResource),
                                    0);

    // However, the pointer and length given are NOT guaranteed to be zero-terminated
    // and most uses of this data will probably want a zero-terminated string.
    // So we're going to construct and return a std::wstring copy from the pointer/length
    // since those are certainly zero-terminated.
    return { readOnlyResource, gsl::narrow<size_t>(length) };
}

// Routine Description:
// - Takes an image architecture and locates a string resource that maps to that architecture.
// Arguments:
// - imageArchitecture - An IMAGE_FILE_MACHINE architecture enum value
//                     - See https://docs.microsoft.com/en-us/windows/win32/sysinfo/image-file-machine-constants
// Return Value:
// - A string value representing the human-readable name of this architecture.
static std::wstring ImageArchitectureToString(USHORT imageArchitecture)
{
    // clang-format off
    const auto id = imageArchitecture == IMAGE_FILE_MACHINE_I386 ? IDS_X86_ARCHITECTURE :
                    imageArchitecture == IMAGE_FILE_MACHINE_AMD64 ? IDS_AMD64_ARCHITECTURE :
                    imageArchitecture == IMAGE_FILE_MACHINE_ARM64 ? IDS_ARM64_ARCHITECTURE :
                    imageArchitecture == IMAGE_FILE_MACHINE_ARM ? IDS_ARM_ARCHITECTURE :
                    IDS_UNKNOWN_ARCHITECTURE;
    // clang-format on

    return GetStringResource(id);
}

// Routine Description:
// - Blocks the user from launching the application with a message box dialog and early exit
//   if the process architecture doesn't match the system platform native architecture.
// - This is because the conhost.exe must match the condrv.sys on the system and the PTY
//   infrastructure that powers everything won't work if we have a mismatch.
// Arguments:
// - <none>
// Return Value:
// - <none>
static void EnsureNativeArchitecture()
{
    USHORT processMachine{};
    USHORT nativeMachine{};
    THROW_IF_WIN32_BOOL_FALSE(IsWow64Process2(GetCurrentProcess(), &processMachine, &nativeMachine));
    if (processMachine != IMAGE_FILE_MACHINE_UNKNOWN && processMachine != nativeMachine)
    {
        const auto formatPattern = GetStringResource(IDS_ERROR_ARCHITECTURE_FORMAT);

        const auto nativeArchitecture = ImageArchitectureToString(nativeMachine);
        const auto processArchitecture = ImageArchitectureToString(processMachine);

        auto buffer{ wil::str_printf<std::wstring>(formatPattern.data(), nativeArchitecture.data(), processArchitecture.data()) };

        MessageBoxW(nullptr,
                    buffer.data(),
                    GetStringResource(IDS_ERROR_DIALOG_TITLE).data(),
                    MB_OK | MB_ICONERROR);

        ExitProcess(0);
    }
}

// // args::Group arguments("arguments");
// // args::ValueFlag<std::string> gitdir(arguments, "path", "", { "git-dir" });
// // args::HelpFlag h(arguments, "help", "help", { 'h', "help" });
// // args::PositionalList<std::string> pathsList(arguments, "paths", "files to commit");

// void _ParseArgs(int w_argc, wchar_t* w_argv[], wchar_t* w_envp[])
// {
//     w_envp;
//     // auto originalCommandline = GetCommandLineW();
//     // int wargc = 0;
//     // wchar_t** wargv = CommandLineToArgvW(originalCommandline, &wargc);
//     // wargv;
//     // wargc;

//     // char* argv = new char[argc];
//     // for (int i = 0; i < argc; i++)
//     // {

//     // }

//     // This is horrifying
//     char** argv = new char*[w_argc];
//     for (int i = 0; i < w_argc; i++)
//     {
//         auto lgth = wcslen(w_argv[i]);
//         argv[i] = new char[lgth + 1];
//         for (int j = 0; j <= lgth; j++)
//         {
//             argv[i][j] = char(w_argv[i][j]);
//         }
//     }

//     auto addParser = [&](args::Subparser& parser) {
//         parser.Parse();
//         std::cout << "Add";
//         // for (auto&& path : pathsList)
//         // {
//         //     std::cout << ' ' << path;
//         // }
//         std::cout << std::endl;
//     };
//     auto commitParser = [&](args::Subparser& parser) {
//         args::ValueFlag<std::string> message(parser, "MESSAGE", "commit message", { 'm' });
//         parser.Parse();
//         std::cout << "Commit";
//         // for (auto&& path : pathsList)
//         // {
//         //     std::cout << ' ' << path;
//         // }
//         std::cout << std::endl;
//         if (message)
//         {
//             std::cout << "message: " << args::get(message) << std::endl;
//         }
//     };

//     args::ArgumentParser rootParser("git-like parser");

//     args::Group commands(rootParser, "commands");
//     args::Command add(commands, "add", "add file contents to the index", addParser);
//     args::Command commit(commands, "commit", "record changes to the repository", commitParser);

//     args::Group arguments("arguments");
//     args::ValueFlag<std::string> gitdir(arguments, "path", "", { "git-dir" });
//     args::HelpFlag h(arguments, "help", "help", { 'h', "help" });
//     args::PositionalList<std::string> pathsList(arguments, "paths", "files to commit");

//     args::GlobalOptions globals(rootParser, arguments);

//     // auto argv = __argv;
//     // auto argc = __argc;
//     if (w_argc == 0)
//     {
//         std::cout << rootParser;
//         exit(0);
//     }

//     try
//     {
//         rootParser.ParseCLI(w_argc, argv);
//         // rootParser.ParseCLI(__argc, __argv);
//     }
//     catch (args::Help)
//     {
//         std::cout << rootParser;
//     }
//     catch (args::Error& e)
//     {
//         std::cerr << e.what() << std::endl
//                   << rootParser;
//         // return 1;
//     }
//     // return 0;
//     exit(0);
// }
int _ParseArgs2(int argc, wchar_t* argv[], wchar_t* envp[]);

// int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
int __stdcall wmain(int argc, wchar_t* argv[], wchar_t* envp[])
{
    TraceLoggingRegister(g_hWindowsTerminalProvider);
    TraceLoggingWrite(
        g_hWindowsTerminalProvider,
        "ExecutableStarted",
        TraceLoggingDescription("Event emitted immediately on startup"),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
        TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));

    _ParseArgs2(argc, argv, envp);

    // Block the user from starting if they launched the incorrect architecture version of the project.
    // This should only be applicable to developer versions. The package installation process
    // should choose and install the correct one from the bundle.
    EnsureNativeArchitecture();

    // Make sure to call this so we get WM_POINTER messages.
    EnableMouseInPointer(true);

    // Create the AppHost object, which will create both the window and the
    // Terminal App. This MUST BE constructed before the Xaml manager as TermApp
    // provides an implementation of Windows.UI.Xaml.Application.
    AppHost host;

    // !!! LOAD BEARING !!!
    // This is _magic_. Do the initial loading of our settings *BEFORE* we
    // initialize our COM apartment type. This is because the Windows.Storage
    // APIs require a MTA. However, other api's (notably the clipboard ones)
    // require that the main thread is an STA. During startup, we don't yet have
    // a dispatcher to background any async work, and we don't want to - we want
    // to load the settings synchronously. Fortunately, WinRT will assume we're
    // in a MTA until we explicitly call init_apartment. We can only call
    // init_apartment _once_, so we'll do the settings loading first, in the
    // implicit MTA, then set our apartment type to STA. The AppHost ctor will
    // load the settings for us, as it constructs the window.
    // This works because Kenny Kerr said it would, and he wrote cpp/winrt, so he knows.
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    // Initialize the xaml content. This must be called AFTER the
    // WindowsXamlManager is initalized.
    host.Initialize();

    MSG message;

    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return 0;
}

struct Cmdline
{
    std::vector<std::wstring> wargs;
    char** _argv = nullptr;
    size_t argc() const { return wargs.size(); }
    char** Argv() const { return _argv; }
    char** BuildArgv()
    {
        // If we've already build our array of args, then we don't need to worry
        // about this. Just return the last one we build.
        if (_argv)
        {
            return _argv;
        }

        // This is horrifying
        _argv = new char*[argc()];
        for (int i = 0; i < argc(); i++)
        {
            auto len = wargs[i].size();
            _argv[i] = new char[len + 1];
            for (int j = 0; j <= len; j++)
            {
                _argv[i][j] = char(wargs[i][j]);
            }
        }
        return _argv;
    }
};

class AppCommandline
{
public:
    AppCommandline();
    ~AppCommandline() = default;
    int ParseCommand(const Cmdline& command);

private:
    void _BuildParser();
    bool _NoCommandsProvided();
    void _ResetStateToDefault();

    CLI::App _app{ "yeet, a test of the wt commandline" };
    CLI::App* _newTabCommand;
    CLI::App* _listProfilesCommand;

    std::string _profileName;
    std::string _startingDirectory;
    std::vector<std::string> _commandline;
    // Are you adding more args here? Make sure to reset them in _ResetStateToDefault
};

std::vector<Cmdline> BuildCommands(int w_argc, wchar_t* w_argv[])
{
    std::wstring cmdSeperator = L";";
    std::vector<Cmdline> commands;
    commands.emplace_back(Cmdline{});
    for (auto i = 0; i < w_argc; i++)
    {
        const auto nextFullArg = std::wstring{ w_argv[i] };
        auto nextDelimiter = nextFullArg.find(cmdSeperator);
        if (nextDelimiter == std::wstring::npos)
        {
            commands.rbegin()->wargs.emplace_back(nextFullArg);
        }
        else
        {
            auto remaining = nextFullArg;
            auto nextArg = remaining.substr(0, nextDelimiter);
            remaining = remaining.substr(nextDelimiter + 1);
            commands.rbegin()->wargs.emplace_back(nextArg);
            do
            {
                // TODO: For delimiters that are escaped, skip them and go to the next
                nextDelimiter = remaining.find(cmdSeperator);
                commands.emplace_back(Cmdline{});
                commands.rbegin()->wargs.emplace_back(std::wstring{ L"wt.exe" });
                nextArg = remaining.substr(0, nextDelimiter);
                commands.rbegin()->wargs.emplace_back(nextArg);
                remaining = remaining.substr(nextDelimiter + 1);
            } while (nextDelimiter != std::wstring::npos);
        }
    }

    return commands;
}

AppCommandline::AppCommandline()
{
    _BuildParser();
    _ResetStateToDefault();
}

void AppCommandline::_ResetStateToDefault()
{
    _profileName = "";
    _startingDirectory = "";
    _commandline.clear();
}

int AppCommandline::ParseCommand(const Cmdline& command)
{
    int localArgc = static_cast<int>(command.argc());
    auto localArgv = command.Argv();

    // std::cout << "######################### starting command #########################\n";
    // for (int i = 0; i < localArgc; i++)
    // {
    //     char* arg = localArgv[i];
    //     std::cout << "arg[" << i << "]=\"";
    //     printf("%s\"\n", arg);
    // }
    _ResetStateToDefault();

    try
    {
        if (localArgc == 2 && (std::string("/?") == localArgv[1] || std::string("-?") == localArgv[1]))
        {
            throw CLI::CallForHelp();
        }
        _app.clear();
        _app.parse(localArgc, localArgv);

        if (_NoCommandsProvided())
        {
            // std::cout << "Didn't find _any_ commands, using newTab to parse\n";
            _newTabCommand->clear();
            _newTabCommand->parse(localArgc, localArgv);
        }
    }
    catch (const CLI::CallForHelp& e)
    {
        return _app.exit(e);
    }
    catch (const CLI::ParseError& e)
    {
        if (_NoCommandsProvided())
        {
            // std::cout << "EXCEPTIONALLY Didn't find _any_ commands, using newTab to parse\n";
            try
            {
                _newTabCommand->clear();
                _newTabCommand->parse(localArgc, localArgv);
            }
            catch (const CLI::ParseError& e)
            {
                return _newTabCommand->exit(e);
            }
        }
        else
        {
            return _app.exit(e);
        }
    }
    return 0;
}

void AppCommandline::_BuildParser()
{
    // app{ "yeet, a test of the wt commandline" };

    ////////////////////////////////////////////////////////////////////////////
    _newTabCommand = _app.add_subcommand("new-tab", "Create a new tab");
    _newTabCommand->add_option("cmdline", _commandline, "Commandline to run in the given profile");
    _newTabCommand->add_option("-p,--profile", _profileName, "Open with the give profile");
    _newTabCommand->add_option("-d,--startingDirectory", _startingDirectory, "Open in the given directory instead of the profile's set startingDirectory");
    _newTabCommand->callback([&, this]() {
        std::cout << "######################### new-tab #########################\n";
        if (!_profileName.empty())
        {
            std::cout << "profileName: " << _profileName << std::endl;
        }
        else
        {
            std::cout << "Use the default profile" << std::endl;
        }
        if (!_startingDirectory.empty())
        {
            std::cout << "startingDirectory: " << _startingDirectory << std::endl;
        }
        else
        {
            std::cout << "Use the default startingDirectory" << std::endl;
        }
        if (_commandline.empty())
        {
            std::cout << "Use the default cmdline" << std::endl;
        }
        else
        {
            auto i = 0;
            for (auto arg : _commandline)
            {
                std::cout << "arg[" << i << "]=\"" << arg << "\"\n";
                i++;
            }
        }
    });
    ////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////
    _listProfilesCommand = _app.add_subcommand("list-profiles", "List all the available profiles");
    ////////////////////////////////////////////////////////////////////////////
}

bool AppCommandline::_NoCommandsProvided()
{
    return !(*_listProfilesCommand || *_newTabCommand);
}

int _ParseArgs2(int w_argc, wchar_t* w_argv[], wchar_t* w_envp[])
{
    w_envp;

    auto commands = BuildCommands(w_argc, w_argv);
    AppCommandline appArgs;

    for (auto& cmdBlob : commands)
    {
        cmdBlob.BuildArgv();
        // On one hand, it seems like we should be able to have one
        // AppCommandline for parsing all of them, and collect the results one
        // at a time.
        //
        // On the other hand, re-using a CLI::App seems to leave state from
        // previous parsings around, so we could get mysterious behavior where
        // one command affects the values of the next.
        //
        // From https://cliutils.github.io/CLI11/book/chapters/options.html:
        // > If that option is not given, CLI11 will not touch the initial
        // > value. This allows you to set up defaults by simply setting your
        // > value beforehand.
        //
        // So we pretty much need the to either manually reset the state each
        // command, or build new ones.
        auto result = appArgs.ParseCommand(cmdBlob);
        if (result != 0)
        {
            exit(result);
        }
        // DONT
        // On success, reset the appArgs state, so we can parse again.
        // Instead just do this every ParseCommand()
    }

    // If all the args were successfully parsed, we'll have some commands built in appArgs

    std::cout << "\nThanks for using yeet!\n"
              << std::endl;
    exit(0);
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
}
