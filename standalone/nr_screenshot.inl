// Manual SteamVR mirror capture. Never changes rendering settings.
namespace {
struct NrScreenshot {
    bool active{};
    ULONGLONG due{};
    HANDLE process{};
    std::filesystem::path root;
    std::string status{"Ctrl+Shift+F6: capture without countdown; menu button: five-second countdown"};
};
NrScreenshot nr_screenshot;
void screenshot_finish(const std::string& message) {
    auto& s=nr_screenshot;
    if(s.process) {CloseHandle(s.process);s.process=nullptr;}
    s.active=false;s.status=message;
    if(!s.root.empty()) {try {nr_capture_write(s.root/"host-result.txt",message);} catch(...) {}}
}
constexpr ULONGLONG screenshot_due(ULONGLONG now, bool from_menu) {
    return now + (from_menu ? 5000 : 0);
}
static_assert(screenshot_due(100, false)==100);
static_assert(screenshot_due(100, true)==5100);
void screenshot_start(OverlayUiState& r, const OverlayRuntime& runtime, bool from_menu) {
    auto& s=nr_screenshot;
    if(s.active) {s.status="Screenshot already in progress; additional request ignored (not queued)";return;}
    if(nr_capture.active || nr_benchmark.active) {s.status="Finish the active comparison/benchmark first";return;}
    try {
        const auto dir=nr_capture_module();
        for(const auto* file:{"Capture-NR-Screenshot.ps1","cpvr_fov_image_audit.exe","nr-capture-openvr.txt"})
            if(!std::filesystem::is_regular_file(dir/file)) throw std::runtime_error("Manual capture helper, script or SteamVR path is missing");
        refresh(r,runtime,true);
        static unsigned sequence{};
        s.root=dir/"nr-screenshots"/(std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64())+"-"+std::to_string(++sequence));
        if(!std::filesystem::create_directories(s.root)) throw std::runtime_error("Cannot create screenshot directory");
        const auto accepted=GetTickCount64();
        s.due=screenshot_due(accepted,from_menu);
        nr_capture_write(s.root/"request.txt",std::string("source=")+(from_menu?"menu":"hotkey")+
            "\naccepted_tick_ms="+std::to_string(accepted)+"\ndue_tick_ms="+std::to_string(s.due));
        s.active=true;
        s.status=from_menu ? "Capture armed: close BOTH menus within five seconds. No settings will change." :
            "Starting capture now (no countdown). Image saving follows acquisition.";
    } catch(const std::exception& e) {screenshot_finish(e.what());}
}
}
void tick_nr_screenshot(OverlayUiState& r, const OverlayRuntime& runtime, bool menu_open, bool requested) {
    if(requested) screenshot_start(r,runtime,false);
    auto& s=nr_screenshot;if(!s.active)return;
    try {
        const auto now=GetTickCount64();
        if(s.process) {
            const auto wait=WaitForSingleObject(s.process,0);
            if(wait==WAIT_TIMEOUT) {
                if(now>s.due) s.status="Capture helper still running; see folder for output. Waiting before allowing another capture.";
                return;
            }
            DWORD code=1;GetExitCodeProcess(s.process,&code);
            refresh(r,runtime,true);nr_capture_write(s.root/"after.json",r.snapshot);
            screenshot_finish(code==0 ? "Saved labelled eyes to Pictures/CheekyFoveatedDLSS and game nr-screenshots; raw originals retained." : "Capture/label failed: see capture-error.txt in the screenshot folder");
            return;
        }
        if(now<s.due)return;
        if(menu_open) {screenshot_finish("Cancelled: close the Cheeky menu and press Ctrl+Shift+F6 again");return;}
        refresh(r,runtime,true);nr_capture_write(s.root/"settings.json",r.snapshot);
        const auto dir=nr_capture_module();
        wchar_t system[MAX_PATH]{};
        if(!GetSystemDirectoryW(system,MAX_PATH))throw std::runtime_error("Cannot locate Windows PowerShell");
        const auto exe=std::filesystem::path(system)/"WindowsPowerShell/v1.0/powershell.exe";
        std::wstring args=L"\""+exe.wstring()+L"\" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \""+
            (dir/"Capture-NR-Screenshot.ps1").wstring()+L"\" -CaptureRoot \""+s.root.wstring()+L"\" -GameProcessId "+std::to_wstring(GetCurrentProcessId());
        STARTUPINFOW startup{sizeof(startup)};PROCESS_INFORMATION child{};
        if(!CreateProcessW(exe.c_str(),args.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,dir.c_str(),&startup,&child))
            throw std::runtime_error("Cannot start screenshot helper script");
        CloseHandle(child.hThread);s.process=child.hProcess;s.due=now+30000;
        s.status="Capturing and labelling both eye mirrors...";
    } catch(const std::exception& e) {screenshot_finish(e.what());}
}
void draw_nr_screenshot(OverlayUiState& r,const OverlayRuntime& runtime) {
    if(!ImGui::CollapsingHeader("Manual headset screenshot (Ctrl+Shift+F6)"))return;
    ImGui::TextWrapped("Ctrl+Shift+F6 starts capture without a countdown; close menus first. Only the menu button waits five seconds. Helper startup/readback still takes time; this is not an exact keypress-frame capture. Saves labelled eye mirrors to Pictures/CheekyFoveatedDLSS and game nr-screenshots; original BMPs retained. Eyes are sequential. No settings change.");
    ImGui::BeginDisabled(nr_screenshot.active || nr_capture.active || nr_benchmark.active);
    if(ImGui::Button("Take headset screenshot in 5 seconds"))screenshot_start(r,runtime,true);
    ImGui::EndDisabled();
    ImGui::TextWrapped("%s",nr_screenshot.status.c_str());
    if(!nr_screenshot.root.empty())ImGui::TextWrapped("%s",nr_screenshot.root.string().c_str());
}
