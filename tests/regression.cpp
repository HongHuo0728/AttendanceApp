#include "model/operations.h"
#include "core/status.h"
#include "app/documents.h"
#include "storage/backup_policy.h"

#include "core/encoding.h"
#include "core/text.h"
#include "core/constants.h"
#include "model/state.h"
#include "model/workbook.h"
#include "model/query.h"
#include "model/history.h"
#include "settings/state.h"
#include "settings/preferences.h"
#include "settings/shortcuts.h"
#include "storage/files.h"
#include "storage/workbook_codec.h"
#include "tests/parser_regression.h"
#include "localization/strings.h"
#include "app/recovery.h"
#include "app/records.h"
#include "animation/scheduler.h"
#include "reports/zip.h"
#include "reports/powerpoint.h"
#include "reports/jobs.h"
#include "reports/csv.h"
#include "reports/html.h"
#include "reports/database.h"
using namespace attendance;
#define SHEETS Document().sheets
#define RECORDS Document().Records()
#define LANGUAGE Settings().language
#define FILTER Document().filterText
#define ADVANCED_FILTER Settings().advancedFilterEnabled

#include <iostream>
#include <stdexcept>
#include "translation_cases.h"

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

#include "ui_regression.h"

LRESULT CALLBACK TestReportProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_APP_REPORT_READY) { DispatchReportCompletions(); return 0; }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void TestBackgroundReports(const std::filesystem::path& dir) {
    WNDCLASSW wc{};
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpfnWndProc = TestReportProc;
    wc.lpszClassName = L"AttendanceReportRegression";
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0,0,0,0,
        HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    Check(hwnd != nullptr, "report message window");
    Check(!QueueReport(hwnd, {}, [](bool) {}), "empty report rejected");
    Check(!QueuePreparedReport(hwnd, []() -> std::function<bool()> { throw std::bad_alloc(); }, [](bool) {}),
        "snapshot allocation failure is reported");
    SHEETS = {MakeSheet(L"Course & test", {{L"2026-10-15 09:00", L"Name <&>", L"Present", L"Notes"}})};
    for(auto key:{L"Leave",L"Pending"})RECORDS.push_back({L"2026-10-15 09:00",key,key,L"reason"});
    LANGUAGE = UiLanguage::English;
    Settings().reportTemplate = ReportTemplate::Complete;
    std::array<std::function<bool()>, 4> tasks = {
        PrepareCsvExport((dir / L"snapshot.csv").wstring()),
        PrepareHtmlExport((dir / L"snapshot.html").wstring()),
        PreparePptxExport((dir / L"snapshot.pptx").wstring()),
        PrepareDatabaseExport(dir / L"snapshot.attddb")};
    SHEETS = {MakeSheet(L"changed after capture")};
    LANGUAGE = UiLanguage::ChineseTraditionalHongKong;
    Settings().reportTemplate = ReportTemplate::Simple;
    int completed = 0;
    bool resultsValid = true;
    const DWORD uiThread = GetCurrentThreadId();
    auto pumpUntil = [&](int target) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (completed < target && std::chrono::steady_clock::now() < deadline) {
            MsgWaitForMultipleObjects(0, nullptr, FALSE, 10, QS_ALLINPUT);
            MSG msg{};
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);
            DispatchReportCompletions();
        }
        return completed == target;
    };
    for (size_t batch = 0; batch < tasks.size(); batch += 2) {
        for (size_t i = batch; i < batch + 2; ++i) {
            Check(QueueReport(hwnd, [task = std::move(tasks[i]), uiThread] {
                return GetCurrentThreadId() != uiThread && task();
            }, [&](bool success) {
                resultsValid = resultsValid && success && GetCurrentThreadId() == uiThread;
                ++completed;
            }), "queue immutable report snapshot");
        }
        Check(!CanQueueReport() && !QueueReport(hwnd, [] { return true; }, [](bool) {}), "bounded report queue");
        Check(pumpUntil(static_cast<int>(batch + 2)), "report completion delivered");
    }
    Check(resultsValid, "report work and callbacks use the correct threads");
    std::string content;
    Check(ReadBinaryFileLimited(dir / L"snapshot.csv", content, 4096)
        && content.find("Date/Time,Name,Status,Other") != std::string::npos
        && content.find("Name <&>,Present,Notes") != std::string::npos, "CSV snapshot and labels");
    Check(ReadBinaryFileLimited(dir / L"snapshot.html", content, 16384)
        && content.find("<h1>Course &amp; test</h1>") != std::string::npos
        && content.find("Name &lt;&amp;&gt;") != std::string::npos
        && content.find("<td>Present</td>") != std::string::npos, "HTML snapshot, escaping and labels");
    Check(ReadBinaryFileLimited(dir / L"snapshot.pptx", content, 4 * 1024 * 1024)
        && content.find("Course &amp; test") != std::string::npos, "PowerPoint immutable snapshot");
    Check(ReadBinaryFileLimited(dir / L"snapshot.attddb", content, 4096)
        && content.find("Course & test") != std::string::npos
        && content.find("Name <&>\tPresent\tNotes") != std::string::npos, "database immutable snapshot");
    Check(QueueReport(hwnd, []() -> bool { throw std::runtime_error("injected report failure"); }, [&](bool success) {
        resultsValid = resultsValid && !success;
        ++completed;
    }), "queue failing report");
    Check(pumpUntil(5) && resultsValid, "report failure reaches UI without success");
    int shutdownCallbacks = 0;
    Check(QueueReport(hwnd, [dir] { return WriteBinaryFileAtomically(dir / L"exit-report.txt", "complete"); },
        [&](bool) { ++shutdownCallbacks; }), "queue report before shutdown");
    StopReportJobs();
    DispatchReportCompletions();
    Check(shutdownCallbacks == 0 && !CanQueueReport()
        && ReadBinaryFileLimited(dir / L"exit-report.txt", content, 100) && content == "complete",
        "shutdown completes writes without orphan UI callbacks");
    if (hwnd) DestroyWindow(hwnd);
}

std::vector<double> tickTimes;
LRESULT CALLBACK TestAnimationProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_APP_ANIMATION_TICK) {
        AcknowledgeAnimationTick(hwnd);
        tickTimes.push_back(std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void TestAnimationScheduler() {
    WNDCLASSW wc{};
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpfnWndProc = TestAnimationProc;
    wc.lpszClassName = L"AttendanceSchedulerRegression";
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0,0,0,0,
        HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    Check(hwnd != nullptr, "scheduler test window");
    bool valid = true;
    for (int fps : {60,120,240,360}) {
        Settings().scrollAnimationFps = fps;
        tickTimes.clear();
        auto start = std::chrono::steady_clock::now();
        RestartAnimation(hwnd, AnimChannel::SmoothScroll, 1.0, 420);
        while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(480)) {
            MsgWaitForMultipleObjects(0, nullptr, FALSE, 4, QS_ALLINPUT);
            MSG msg{};
            while (PeekMessageW(&msg, nullptr, 0,0, PM_REMOVE)) DispatchMessageW(&msg);
        }
        std::vector<double> intervals;
        for (size_t i = 1; i < tickTimes.size(); ++i) intervals.push_back(tickTimes[i]-tickTimes[i-1]);
        std::sort(intervals.begin(), intervals.end());
        valid = valid && tickTimes.size() >= 8 && tickTimes.size() <= static_cast<size_t>(fps);
        if (!intervals.empty()) std::cout << "scheduler fps=" << fps << " ticks=" << tickTimes.size()
            << " median_ms=" << intervals[intervals.size()/2] << '\n';
    }
    Settings().scrollAnimationFps = 60;
    tickTimes.clear();
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(400)) {
        RestartAnimation(hwnd, AnimChannel::SmoothScroll, 1.0, 200);
        MsgWaitForMultipleObjects(0, nullptr, FALSE, 3, QS_ALLINPUT);
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0,0, PM_REMOVE)) DispatchMessageW(&msg);
    }
    valid = valid && tickTimes.size() >= 5;
    RemoveAnimationState(hwnd);
    StopAnimationThread();
    if (hwnd) DestroyWindow(hwnd);
    Check(valid, "scheduler pacing and wake starvation");
}

template<class F> double Time(F&& work) {
    std::array<double, 5> samples{};
    work();
    for (auto& sample : samples) {
        auto start = std::chrono::steady_clock::now();
        work();
        sample = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    }
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}


void Test210(const std::filesystem::path& dir){
 Document()=DocumentState{};Settings().language=UiLanguage::English;
 auto reset=[](){Document()=DocumentState{};Document().sheets={MakeSheet(L"Course")};};
 reset();auto& records=Document().Records();
 for(auto key:StatusKeys)records.push_back({L"2026-10-20 morning",key,key,key});
 auto stats=CountRecords(records);
 Check(stats.total==6&&stats.Eligible()==4&&stats.leave==1&&stats.pending==1&&AttendanceRateText(stats)==L"25.0%","six status denominator");
 Check(AttendanceRateText(PptStats{})==L"No data","zero denominator has no data");
 Check(!ValidStatusNote(L"Leave",L"  ")&&!ValidStatusNote(L"Other",L"")&&ValidStatusNote(L"Present",L""),"shared required notes");
 Check(DateWithinCalendarWeek(L"2026-10-19",L"2026-10-20")&&DateWithinCalendarWeek(L"2026-10-25",L"2026-10-20")&&!DateWithinCalendarWeek(L"2026-10-18",L"2026-10-20")&&!DateWithinCalendarWeek(L"2026-10-26",L"2026-10-20"),"calendar week edges");
 Check(DateWithinCalendarWeek(L"2026-12-28",L"2027-01-01")&&!DateWithinCalendarWeek(L"2027-01-04",L"2027-01-01"),"calendar week year boundary");
 Document().statsRange=StatsRange::ThisMonth;Check(RecordMatchesStatsRange({TodayDateKey(),L"",L"Present",L""})&&!RecordMatchesStatsRange({L"2026-99-99",L"",L"Present",L""}),"month dates");Document().statsRange=StatsRange::All;
 std::string encoded,plain;Check(EncodeCurrentWorkbook(encoded)&&DecodeAttd(encoded,plain)&&plain.rfind("ATTENDANCE_V6\n",0)==0,"default V6");
 Check(EncodeV5Compatible(encoded)&&DecodeAttd(encoded,plain)&&plain.rfind("ATTENDANCE_V5\n",0)==0,"V5 compatibility export");
 std::vector<AttendanceSheet> parsed;Check(DeserializeWorkbook(plain,parsed)&&parsed[0].records[4].status==L"Other"&&parsed[0].records[4].other.find(L"Leave")!=std::wstring::npos&&parsed[0].records[5].other.find(L"Pending")!=std::wstring::npos,"V5 loss explanation");
 Check(Document().Records()[4].status==L"Leave","V5 export immutable source");
 reset();Document().sheets[0].students={L"Alice",L"Bob",L"Carol"};
 Document().Records()={{L"2026-10-20 morning",L"Alice",L"Pending",L""},{L"2026-10-20 afternoon",L"Alice",L"Absent",L""},{L"2026-10-20 morning",L"Bob",L"Pending",L""}};
 Settings().advancedFilterEnabled=false;Document().lessonFilter=L"2026-10-20 morning";
 Check(CurrentScopeRows()==std::vector<int>({0,2}),"lesson selection independent of advanced syntax and same day sessions");
 auto original=SerializeWorkbook();Check(!ChangeRecords({0,2},L"Leave",L"",NoteMode::Replace)&&SerializeWorkbook()==original,"batch validation transactional");
 Check(ChangeRecords({0,2},L"Leave",L"approved",NoteMode::Replace)&&Document().undoStack.size()==1&&Document().Records()[1].status==L"Absent","filtered batch leaves hidden rows intact");
 UndoLast();Check(SerializeWorkbook()==original,"whole batch undo");RedoLast();Check(Document().Records()[0].status==L"Leave","whole batch redo");
 Check(!ChangeRecords({0},L"",L"",NoteMode::Replace),"notes cannot be cleared for leave");
 Check(ChangeRecords({0},L"Present",L"made up",NoteMode::Append)&&Document().Records()[0].other==L"approved; made up","append ordinary notes");
 Check(!MakeUpRecord(L"missing",L"Carol",L"Present",L"reason")&&!MakeUpRecord(L"2026-10-20 morning",L"Carol",L"Present",L" "),"make-up validates lesson and reason");
 Check(MakeUpRecord(L"2026-10-20 morning",L"Carol",L"Present",L"teacher correction"),"make-up missing roster member");
 Check(MakeUpRecord(L"2026-10-20 morning",L"Alice",L"Late",L"corrected")&&Document().Records()[0].status==L"Late","make-up unique update");
 Document().Records().push_back(Document().Records()[0]);original=SerializeWorkbook();Check(!MakeUpRecord(L"2026-10-20 morning",L"Alice",L"Present",L"reason")&&SerializeWorkbook()==original,"duplicate make-up rejected without mutation");Document().Records().pop_back();
 Check(!RenameLesson(L"2026-10-20 morning",L"2026-10-20 afternoon"),"lesson rename collision");
 Check(RenameLesson(L"2026-10-20 morning",L"中文课次 with spaces"),"rename spaced unicode lesson");
 Check(CopyLesson(L"中文课次 with spaces",L"new lesson"),"copy lesson");
 size_t copies=0;for(const auto& r:Document().Records())if(r.dateTime==L"new lesson"){++copies;Check(r.status==L"Pending"&&r.other.empty(),"copied lesson resets status and notes");}Check(copies==3,"copied lesson members");
 Check(!CopyLesson(L"中文课次 with spaces",L"new lesson"),"copy collision rejected");
 Document().sheets[0].teacher=L"Teacher";Check(CreateCourseFromTemplate(0,L"New course")&&Document().activeSheet==1&&Document().Records().empty()&&Document().sheets[1].students.size()==3&&Document().sheets[1].teacher==L"Teacher","course template excludes records");
 UndoLast();Check(Document().sheets.size()==1,"course template undo");RedoLast();Check(Document().sheets.size()==2,"course template redo");
 auto migration=dir/L"old.attd";Check(EncodeWorkbookData(Document().sheets,0,encoded,5)&&WriteBinaryFileAtomically(migration,encoded),"migration fixture");
 Check(EnsureMigrationBackup(migration)&&ReadBinaryFileLimited(std::filesystem::path(migration.wstring()+L".pre-v6.bak"),plain,100000)&&plain==encoded,"old file protected before overwrite");
 auto blocked=dir/L"blocked.attd";Check(WriteBinaryFileAtomically(blocked,encoded),"blocked migration fixture");std::filesystem::create_directory(std::filesystem::path(blocked.wstring()+L".pre-v6.bak"));
 Check(!EnsureMigrationBackup(blocked)&&ReadBinaryFileLimited(blocked,plain,100000)&&plain==encoded,"backup failure preserves old file");
 auto backups=dir/L"quota";std::filesystem::create_directory(backups);
 Check(WriteBinaryFileAtomically(backups/L"autosave-protected-1.attd",std::string(20,'P')),"protected quota fixture");
 for(int i=0;i<11;++i)Check(WriteBinaryFileAtomically(backups/(L"backup-"+std::to_wstring(i)+L".attd"),std::string(10,'B')),"small backup fixture");
 Check(ReserveBackupSpace(backups,10,true,160,3),"backup count rotation");
 size_t count=0;for(auto& entry:std::filesystem::directory_iterator(backups))if(entry.path().filename().wstring().rfind(L"backup-",0)==0)++count;
 Check(count<=2&&ReadBinaryFileLimited(backups/L"autosave-protected-1.attd",plain,100)&&plain==std::string(20,'P'),"protected backups survive rotation");
 Check(!ReserveBackupSpace(backups,1,false,20,3)&&FileExistsNoThrow(backups/L"autosave-protected-1.attd"),"protected capacity failure preserves recovery");
 reset();Settings().advancedFilterEnabled=true;
}
int wmain(int argc, wchar_t** argv) {
    try {
        Check(argc == 2, "isolated data directory required");
        std::filesystem::path dir(argv[1]);
        std::filesystem::create_directories(dir);
        SetEnvironmentVariableW(L"ATTENDANCEAPP_DATA_DIR", dir.c_str());
        Test210(dir);
        TestUiRedraw();
        Check(RunParserRegressionChecks(), "existing parser regression");
        for (int version = 1; version <= 6; ++version) {
            std::string plain = "ATTENDANCE_V" + std::to_string(version) + "\n";
            if (version >= 3) plain += "0\n";
            plain += "1\n";
            if (version >= 2) plain += "Course\n";
            if (version >= 4) plain += "Teacher\nRoom\nNotes\n1\nStudent\n";
            if (version >= 2) plain += "1\n";
            plain += "2026-10-15 09:00\tStudent\tPresent\t\n";
            std::vector<AttendanceSheet> output;
            Check(DeserializeWorkbook(plain, output) && output.size() == 1
                && output[0].records.size() == 1, "V1-V6 compatibility");
        }
        for (int language = 0; language < 20; ++language) {
            LANGUAGE = static_cast<UiLanguage>(language);
            uint64_t hash = 14695981039346656037ull;
            for (const auto& item : translationCases) {
                // This label intentionally changes with the approved FPS limit.
                if (std::wstring(item.english) == L"Dropdown scroll frame rate (60-240)") continue;
                auto text = Tr(item.english, item.chinese);
                Check(!text.empty(), "empty translation");
                for (wchar_t ch : text) { hash ^= static_cast<uint16_t>(ch); hash *= 1099511628211ull; }
                hash ^= 0xffff; hash *= 1099511628211ull;
            }
            std::cout << "translation " << language << " " << hash << '\n';
            Check(ScrollFrameRateLabel().find(L"360") != std::wstring::npos, "localized FPS upper bound");
        }
        LANGUAGE = UiLanguage::English;
        for (int count : {1, 10, 100}) {
            SHEETS = {MakeSheet(L"Course")};
            RECORDS.clear();
            for (int i = 0; i < count; ++i) {
                auto name = L"Student " + std::to_wstring(i);
                SHEETS[0].students.push_back(name);
                for (int day = 1; day <= 10; ++day)
                    RECORDS.push_back({L"2026-10-" + std::to_wstring(day) + L" 09:00", name,
                        i % 3 ? L"Present" : L"Absent", L"notes & <test>"});
            }
            std::string plain, payload, decoded;
            std::vector<AttendanceSheet> parsed;
            auto serialize = Time([&] { plain = SerializeWorkbook(); });
            auto encode = Time([&] { Check(EncodeCurrentWorkbook(payload), "encode"); });
            auto decode = Time([&] { Check(DecodeAttd(payload, decoded), "decode"); });
            auto parse = Time([&] { Check(DeserializeWorkbook(decoded, parsed), "parse"); });
            Check(plain == decoded && parsed[0].records.size() == RECORDS.size(), "round trip");
            FILTER = L"name:student status:present";
            ADVANCED_FILTER = true;
            size_t matches = 0;
            auto search = Time([&] { matches = 0; for (const auto& r : RECORDS) if (RecordMatchesFilter(r)) ++matches; });
            Check(matches == static_cast<size_t>(count - (count + 2) / 3) * 10, "filter result");
            std::cout << "benchmark students=" << count << " records=" << RECORDS.size()
                << " serialize_ms=" << serialize << " encode_ms=" << encode
                << " decode_ms=" << decode << " parse_ms=" << parse << " search_ms=" << search;
            auto ppt = Time([&] { Check(ExportPptxFile((dir / L"report.pptx").wstring()), "PowerPoint export"); });
            std::cout << " pptx_ms=" << ppt << '\n';
            std::string pptBytes;
            Check(ReadBinaryFileLimited(dir / L"report.pptx", pptBytes, 4 * 1024 * 1024), "read generated PowerPoint");
            Check(pptBytes.find("<a:ext cx=\"-") == std::string::npos
                && PptStatusBarWidth(INT_MAX,INT_MAX)==6000000 && PptStatusBarWidth(-1,100)==0,
                "large attendance counts preserve chart dimensions");
        }
        TestAnimationScheduler();
        SHEETS = {MakeSheet(L"Course", {{L"1",L"A",L"Present",L""},
            {L"2",L"B",L"Absent",L""}, {L"3",L"C",L"Late",L""}})};
        Document().undoStack.clear(); Document().redoStack.clear();
        EraseRows({-1,0,0,2,999});
        Check(RECORDS.size() == 1 && RECORDS[0].name == L"B", "batch delete valid unique indexes");
        UndoLast();
        Check(RECORDS.size() == 3 && RECORDS[0].name == L"A", "whole operation undo");
        RedoLast();
        Check(RECORDS.size() == 1 && RECORDS[0].name == L"B", "whole operation redo");
        Check(NormalizeShortcut(L"Ctrl+F1junk", L"Ctrl+S") == L"Ctrl+S", "shortcut strict parsing");
        auto file = dir / L"atomic.txt";
        Check(WriteBinaryFileAtomically(file, "original"), "atomic initial write");
        Check(!WriteStreamFileAtomically(file, [](std::ofstream& out) { out << "partial"; return false; }), "writer failure");
        Check(!WriteStreamFileAtomically(file, [](std::ofstream&) -> bool { throw std::runtime_error("injected"); }), "writer exception");
        std::string content;
        Check(ReadBinaryFileLimited(file, content, 100) && content == "original", "failed write preserves original");
        Check(!ReadBinaryFileLimited(file, content, 2) && content.empty(), "bounded file read");
        std::atomic_bool concurrentWritesSucceeded{true};
        auto writeMany = [&](char ch) {
            for (int i=0; i<20; ++i)
                if (!WriteBinaryFileAtomically(file, std::string(4096,ch))) concurrentWritesSucceeded = false;
        };
        std::thread first(writeMany,'A'), second(writeMany,'B');
        first.join(); second.join();
        Check(concurrentWritesSucceeded && ReadBinaryFileLimited(file,content,8192)
            && (content == std::string(4096,'A') || content == std::string(4096,'B')), "concurrent atomic writers");
        for (const auto& item : std::filesystem::directory_iterator(dir))
            Check(item.path().filename().wstring().find(L".tmp-") == std::wstring::npos, "temporary cleanup");
        for (int fps : {60,75,90,120,144,165,240,360}) {
            Check(WriteBinaryFileAtomically(dir / L"settings.ini", "advanced_personalization=0\ntheme=light\nscroll_animation_fps=" + std::to_string(fps) + "\n"), "settings fixture");
            LoadSettings();
            Check(Settings().theme == UiTheme::Light && Settings().scrollAnimationFps == fps, "legacy gate and FPS migration");
        }
        for (const auto& value : {"", "invalid", "-1", "59", "361", "999999999999999999999999999"}) {
            Check(WriteBinaryFileAtomically(dir / L"settings.ini", std::string("scroll_animation_fps=") + value + "\n"), "invalid FPS fixture");
            LoadSettings();
            Check(Settings().scrollAnimationFps >= 60 && Settings().scrollAnimationFps <= 360, "invalid FPS safely bounded");
        }
        Settings().autosavePromptEnabled = false;
        Check(WriteBinaryFileAtomically(dir / L"autosave.attd", "protected recovery data"), "recovery fixture");
        PromptRestoreAutosave();
        Check(!Document().allowAutosaveOverwrite, "disabled prompt protects old recovery");
        Check(ClearAutosaveFile(), "safe recovery cleanup");
        bool protectedCopy = false;
        for (const auto& item : std::filesystem::directory_iterator(dir / L"backups")) {
            if (ReadBinaryFileLimited(item.path(), content, 100) && content == "protected recovery data") protectedCopy = true;
        }
        Check(protectedCopy, "ignored recovery archived before cleanup");
        MarkDirty();
        Check(AutoSaveNow() == AutosaveResult::Saved, "autosave changed revision");
        Check(AutoSaveNow() == AutosaveResult::Skipped, "autosave unchanged revision");
        MarkDirty();
        Check(AutoSaveNow() == AutosaveResult::Saved, "autosave subsequent edit");
        Check(WriteZipStore(dir / L"report-test.zip", {{"report.txt", "123456789",0,0}}), "ZIP writer");
        Check(ReadBinaryFileLimited(dir / L"report-test.zip", content, 4096), "ZIP read");
        Check(content.size() > 18 && static_cast<unsigned char>(content[14]) == 0x26
            && static_cast<unsigned char>(content[15]) == 0x39
            && static_cast<unsigned char>(content[16]) == 0xf4
            && static_cast<unsigned char>(content[17]) == 0xcb, "CRC32 known vector");
        TestBackgroundReports(dir);
        std::cout << "All regression checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
