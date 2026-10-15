#include "reports/powerpoint.h"
#include "model/query.h"
#include "localization/strings.h"
#include "model/workbook.h"
#include "ui/dialogs.h"
#include "ui/file_dialogs.h"
#include "core/encoding.h"
#include "reports/zip.h"
#include "model/state.h"
#include "settings/state.h"
#include "core/constants.h"
#include "reports/jobs.h"
#include "ui/state.h"

namespace attendance {

std::string ShapeText(int id, int x, int y, int cx, int cy, const std::wstring& text, int size, const char* color, bool bold = false);
std::string RectShape(int id, int x, int y, int cx, int cy, const char* fill, const char* line = "FFFFFF", int alpha = 100000);
std::string LineShape(int id, int x1, int y1, int x2, int y2, const char* color, int width = 36000);
std::string TriangleShape(int id, int x1, int y1, int x2, int y2, int x3, int y3, const char* fill);
std::string EllipseShape(int id, int x, int y, int cx, int cy, const char* fill, const char* line = "FFFFFF", int alpha = 100000);
void AddPieShape(std::ostringstream& ss, int& id, const PptStats& stats, int cx, int cy, int radius);
std::string SlideBackground();
std::string SlideXml(const std::string& body);
std::string BuildPptSlide1(const AttendanceSheet& sheet, const PptStats& stats);
std::string BuildPptSlide2(const PptStats& stats);
std::string BuildPptSlide3(const std::vector<AttendanceRecord>& records);
std::string PptContentTypes();
std::string PptDefaultTextStyleXml();
std::string PptPresPropsXml();
std::string PptViewPropsXml();
std::string PptTableStylesXml();
std::string PptThemeXml();
std::string PptSlideLayoutXml();
std::string PptSlideMasterXml();
bool ExportPptxFile(const std::wstring& path);

std::string ShapeText(int id, int x, int y, int cx, int cy, const std::wstring& text, int size, const char* color, bool bold){
    std::ostringstream ss;
    std::string languageTag = LanguageToString(Settings().language);
    if (languageTag == "en") languageTag = "en-US";
    ss << "<p:sp><p:nvSpPr><p:cNvPr id=\"" << id << "\" name=\"Text" << id
       << "\"/><p:cNvSpPr txBox=\"1\"/><p:nvPr/></p:nvSpPr><p:spPr><a:xfrm><a:off x=\"" << x << "\" y=\"" << y
       << "\"/><a:ext cx=\"" << cx << "\" cy=\"" << cy << "\"/></a:xfrm><a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>"
       << "<a:noFill/><a:ln><a:noFill/></a:ln></p:spPr><p:txBody><a:bodyPr wrap=\"square\"/><a:lstStyle/><a:p>"
       << "<a:r><a:rPr lang=\"" << languageTag << "\" sz=\"" << size << "\"" << (bold ? " b=\"1\"" : "") << "><a:solidFill><a:srgbClr val=\""
       << color << "\"/></a:solidFill></a:rPr><a:t>" << XmlCell(text) << "</a:t></a:r></a:p></p:txBody></p:sp>";
    return ss.str();
}

std::string RectShape(int id, int x, int y, int cx, int cy, const char* fill, const char* line, int alpha){
    std::ostringstream ss;
    ss << "<p:sp><p:nvSpPr><p:cNvPr id=\"" << id << "\" name=\"Rect" << id
       << "\"/><p:cNvSpPr/><p:nvPr/></p:nvSpPr><p:spPr><a:xfrm><a:off x=\"" << x << "\" y=\"" << y
       << "\"/><a:ext cx=\"" << cx << "\" cy=\"" << cy << "\"/></a:xfrm><a:prstGeom prst=\"roundRect\"><a:avLst/></a:prstGeom>"
       << "<a:solidFill><a:srgbClr val=\"" << fill << "\"><a:alpha val=\"" << alpha << "\"/></a:srgbClr></a:solidFill>"
       << "<a:ln w=\"12000\"><a:solidFill><a:srgbClr val=\"" << line << "\"/></a:solidFill></a:ln></p:spPr></p:sp>";
    return ss.str();
}

std::string LineShape(int id, int x1, int y1, int x2, int y2, const char* color, int width){
    std::ostringstream ss;
    bool flipH = x2 < x1;
    bool flipV = y2 < y1;
    ss << "<p:cxnSp><p:nvCxnSpPr><p:cNvPr id=\"" << id << "\" name=\"Line" << id
       << "\"/><p:cNvCxnSpPr/><p:nvPr/></p:nvCxnSpPr><p:spPr><a:xfrm";
    if (flipH) ss << " flipH=\"1\"";
    if (flipV) ss << " flipV=\"1\"";
    ss << "><a:off x=\"" << std::min(x1, x2) << "\" y=\"" << std::min(y1, y2)
       << "\"/><a:ext cx=\"" << std::max(1, std::abs(x2 - x1)) << "\" cy=\"" << std::max(1, std::abs(y2 - y1)) << "\"/></a:xfrm>"
       << "<a:prstGeom prst=\"line\"><a:avLst/></a:prstGeom><a:ln w=\"" << width << "\"><a:solidFill><a:srgbClr val=\""
       << color << "\"/></a:solidFill></a:ln></p:spPr></p:cxnSp>";
    return ss.str();
}

std::string TriangleShape(int id, int x1, int y1, int x2, int y2, int x3, int y3, const char* fill){
    std::ostringstream ss;
    ss << "<p:sp><p:nvSpPr><p:cNvPr id=\"" << id << "\" name=\"PieSlice" << id
       << "\"/><p:cNvSpPr/><p:nvPr/></p:nvSpPr><p:spPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"12192000\" cy=\"6858000\"/></a:xfrm>"
       << "<a:custGeom><a:avLst/><a:gdLst/><a:ahLst/><a:cxnLst/><a:rect l=\"0\" t=\"0\" r=\"12192000\" b=\"6858000\"/>"
       << "<a:pathLst><a:path w=\"12192000\" h=\"6858000\"><a:moveTo><a:pt x=\"" << x1 << "\" y=\"" << y1
       << "\"/></a:moveTo><a:lnTo><a:pt x=\"" << x2 << "\" y=\"" << y2 << "\"/></a:lnTo><a:lnTo><a:pt x=\""
       << x3 << "\" y=\"" << y3 << "\"/></a:lnTo><a:close/></a:path></a:pathLst></a:custGeom>"
       << "<a:solidFill><a:srgbClr val=\"" << fill << "\"/></a:solidFill><a:ln><a:noFill/></a:ln></p:spPr></p:sp>";
    return ss.str();
}

std::string EllipseShape(int id, int x, int y, int cx, int cy, const char* fill, const char* line, int alpha){
    std::ostringstream ss;
    ss << "<p:sp><p:nvSpPr><p:cNvPr id=\"" << id << "\" name=\"Ellipse" << id
       << "\"/><p:cNvSpPr/><p:nvPr/></p:nvSpPr><p:spPr><a:xfrm><a:off x=\"" << x << "\" y=\"" << y
       << "\"/><a:ext cx=\"" << cx << "\" cy=\"" << cy << "\"/></a:xfrm><a:prstGeom prst=\"ellipse\"><a:avLst/></a:prstGeom>"
       << "<a:solidFill><a:srgbClr val=\"" << fill << "\"><a:alpha val=\"" << alpha << "\"/></a:srgbClr></a:solidFill>"
       << "<a:ln w=\"18000\"><a:solidFill><a:srgbClr val=\"" << line << "\"/></a:solidFill></a:ln></p:spPr></p:sp>";
    return ss.str();
}

void AddPieShape(std::ostringstream& ss, int& id, const PptStats& stats, int cx, int cy, int radius){
    int values[] = {stats.present, stats.absent, stats.late, stats.other};
    const char* colors[] = {"8CA39A", "A67575", "A89876", "9A9A9A"};
    if (stats.total <= 0) {
        ss << EllipseShape(id++, cx - radius, cy - radius, radius * 2, radius * 2, "1A1A1A", "E0E0E0", 100000);
        return;
    }

    double angle = -90.0;
    for (int i = 0; i < 4; ++i) {
        if (values[i] <= 0) continue;
        double sweep = values[i] * 360.0 / stats.total;
        int steps = std::max(1, (int)std::ceil(sweep / 8.0));
        for (int step = 0; step < steps; ++step) {
            double a1 = angle + sweep * step / steps - (step > 0 ? 0.55 : 0.0);
            double a2 = angle + sweep * (step + 1) / steps + (step + 1 < steps ? 0.55 : 0.0);
            double r1 = a1 * 3.14159265358979323846 / 180.0;
            double r2 = a2 * 3.14159265358979323846 / 180.0;
            int x2 = cx + (int)std::round(radius * std::cos(r1));
            int y2 = cy + (int)std::round(radius * std::sin(r1));
            int x3 = cx + (int)std::round(radius * std::cos(r2));
            int y3 = cy + (int)std::round(radius * std::sin(r2));
            ss << TriangleShape(id++, cx, cy, x2, y2, x3, y3, colors[i]);
        }
        angle += sweep;
    }
    ss << EllipseShape(id++, cx - radius, cy - radius, radius * 2, radius * 2, "FFFFFF", "E0E0E0", 0);
}

std::string SlideBackground(){
    return "<p:sp><p:nvSpPr><p:cNvPr id=\"2\" name=\"Background\"/><p:cNvSpPr/><p:nvPr/></p:nvSpPr>"
           "<p:spPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"12192000\" cy=\"6858000\"/></a:xfrm>"
           "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom><a:solidFill><a:srgbClr val=\"000000\"/></a:solidFill><a:ln><a:noFill/></a:ln></p:spPr></p:sp>";
}

std::string SlideXml(const std::string& body){
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
           "<p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"
           + body +
           "</p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sld>";
}

std::string BuildPptSlide1(const AttendanceSheet& sheet, const PptStats& stats){
    int id = 3;
    double rate = stats.total ? stats.present * 100.0 / stats.total : 0.0;
    std::wstringstream rateText;
    rateText.setf(std::ios::fixed);
    rateText.precision(1);
    rateText << rate << L"%";
    std::wstring title = Tr(L"Attendance Manager", L"\u70b9\u540d\u7ba1\u7406\u5668");
    if (Settings().reportTemplate == ReportTemplate::Simple) title = Tr(L"Attendance Summary", L"\u51fa\u52e4\u6458\u8981");
    else if (Settings().reportTemplate == ReportTemplate::Teacher) title = Tr(L"Course Attendance Report", L"\u8bfe\u7a0b\u51fa\u52e4\u62a5\u544a");
    else if (Settings().reportTemplate == ReportTemplate::Parent) title = Tr(L"Student Attendance Report", L"\u5b66\u751f\u51fa\u52e4\u62a5\u544a");

    std::wstring footer = Tr(L"Generated by AttendanceApp", L"\u7531 AttendanceApp \u751f\u6210");
    if (Settings().reportTemplate == ReportTemplate::Teacher || Settings().reportTemplate == ReportTemplate::Complete) {
        footer.clear();
        if (!sheet.teacher.empty()) footer += Tr(L"Teacher/owner:", L"\u6559\u5e08/\u8d1f\u8d23\u4eba\uff1a") + L" " + sheet.teacher + L"  ";
        if (!sheet.location.empty()) footer += Tr(L"Location:", L"\u5730\u70b9\uff1a") + L" " + sheet.location + L"  ";
        if (!sheet.notes.empty()) footer += Tr(L"Course notes:", L"\u8bfe\u7a0b\u5907\u6ce8\uff1a") + L" " + sheet.notes;
        if (footer.empty()) footer = Tr(L"Teacher template with course details enabled.", L"\u6559\u5e08\u7248\u62a5\u544a\u5df2\u542f\u7528\u8bfe\u7a0b\u8be6\u60c5\u3002");
    } else if (Settings().reportTemplate == ReportTemplate::Parent) {
        footer = Tr(L"Roster students", L"\u540d\u5355\u5b66\u751f") + L" " + std::to_wstring(sheet.students.size()) + L" | "
            + Tr(L"Attendance", L"\u51fa\u52e4\u7387") + L" " + rateText.str();
    }

    std::ostringstream ss;
    ss << SlideBackground();
    ss << ShapeText(id++, 760000, 520000, 8000000, 340000, ReportTemplateName(Settings().reportTemplate) + L" | " + StatsRangeName(Document().statsRange), 1300, "BDBDBD", true);
    ss << ShapeText(id++, 740000, 900000, 8400000, 700000, title, 3400, "FFFFFF", true);
    ss << ShapeText(id++, 760000, 1600000, 7600000, 420000, sheet.name, 1600, "E0E0E0");
    ss << RectShape(id++, 760000, 2180000, 10670000, 22000, "BDBDBD", "BDBDBD", 100000);

    ss << RectShape(id++, 800000, 2680000, 4650000, 2450000, "121212", "444444", 93000);
    ss << ShapeText(id++, 1160000, 3020000, 3600000, 360000, Tr(L"Attendance", L"\u51fa\u52e4\u7387"), 1550, "8CA39A", true);
    ss << ShapeText(id++, 1120000, 3420000, 3400000, 920000, rateText.str(), 4400, "FFFFFF", true);
    ss << ShapeText(id++, 1160000, 4520000, 3600000, 340000, Tr(L"Present", L"\u51fa\u5e2d") + L" " + std::to_wstring(stats.present) + L" / " + Tr(L"Total", L"\u603b\u6570") + L" " + std::to_wstring(stats.total), 1250, "B8B8B8", true);

    const wchar_t* labels[] = {L"Total", L"Present", L"Absent", L"Late"};
    int values[] = {stats.total, stats.present, stats.absent, stats.late};
    const char* fills[] = {"1A1A1A", "18221E", "2A1F1F", "292515"};
    for (int i = 0; i < 4; ++i) {
        int x = 5850000 + (i % 2) * 2550000;
        int y = 2680000 + (i / 2) * 1250000;
        ss << RectShape(id++, x, y, 2300000, 960000, fills[i], "444444", 94000);
        ss << ShapeText(id++, x + 240000, y + 170000, 1700000, 280000, Tr(labels[i], labels[i]), 1150, "E0E0E0", true);
        ss << ShapeText(id++, x + 240000, y + 460000, 1700000, 380000, std::to_wstring(values[i]), 2500, "FFFFFF", true);
    }

    ss << ShapeText(id++, 760000, 5950000, 10400000, 340000, footer, 1050, "B8B8B8");
    return SlideXml(ss.str());
}

std::string BuildPptSlide2(const PptStats& stats){
    int id = 3;
    std::ostringstream ss;
    ss << SlideBackground();
    ss << ShapeText(id++, 760000, 520000, 9000000, 520000, Tr(L"Statistics Chart", L"\u7edf\u8ba1\u56fe\u8868"), 2900, "FFFFFF", true);
    ss << ShapeText(id++, 780000, 1050000, 9000000, 300000, Tr(L"Status", L"\u72b6\u6001") + L" " + Tr(L"Statistics chart", L"\u7edf\u8ba1\u56fe\u8868"), 1150, "B8B8B8");
    ss << RectShape(id++, 760000, 1420000, 10670000, 22000, "BDBDBD", "BDBDBD", 100000);
    const wchar_t* labels[] = {L"Present", L"Absent", L"Late", L"Other"};
    int values[] = {stats.present, stats.absent, stats.late, stats.other};
    const char* colors[] = {"8CA39A", "A67575", "A89876", "9A9A9A"};

    ss << RectShape(id++, 760000, 1750000, 4550000, 3900000, "121212", "444444", 93000);
    ss << ShapeText(id++, 1080000, 2020000, 3400000, 330000, Tr(L"Attendance Sheet", L"\u70b9\u540d\u8868"), 1350, "FFFFFF", true);
    ss << EllipseShape(id++, 2000000, 2600000, 1800000, 1800000, "000000", "444444", 100000);
    AddPieShape(ss, id, stats, 2900000, 3500000, 850000);
    ss << RectShape(id++, 4050000, 3050000, 850000, 900000, "1A1A1A", "444444", 86000);
    ss << ShapeText(id++, 4210000, 3220000, 560000, 240000, Tr(L"Total", L"\u603b\u6570"), 850, "B8B8B8", true);
    ss << ShapeText(id++, 4210000, 3500000, 560000, 350000, std::to_wstring(stats.total), 1800, "FFFFFF", true);

    ss << RectShape(id++, 5600000, 1750000, 5830000, 3900000, "121212", "444444", 93000);
    ss << ShapeText(id++, 5920000, 2020000, 2400000, 330000, Tr(L"Total", L"\u603b\u6570") + L" " + std::to_wstring(stats.total), 1350, "FFFFFF", true);
    ss << ShapeText(id++, 9100000, 2020000, 1900000, 330000, Tr(L"Attendance", L"\u51fa\u52e4\u7387"), 1200, "8CA39A", true);
    int attendancePct = stats.total ? (int)std::round(stats.present * 100.0 / stats.total) : 0;
    ss << ShapeText(id++, 10180000, 1940000, 850000, 460000, std::to_wstring(attendancePct) + L"%", 2050, "FFFFFF", true);

    int pctY = 2550000;
    for (int i = 0; i < 4; ++i) {
        int x = 5920000 + (i % 2) * 2650000;
        int y = pctY + (i / 2) * 600000;
        int pct = stats.total ? (int)std::round(values[i] * 100.0 / stats.total) : 0;
        ss << RectShape(id++, x, y, 2350000, 460000, "1A1A1A", "444444", 88000);
        ss << RectShape(id++, x + 180000, y + 115000, 220000, 220000, colors[i], colors[i], 100000);
        ss << ShapeText(id++, x + 500000, y + 70000, 1000000, 240000, Tr(labels[i], labels[i]), 900, "E0E0E0", true);
        ss << ShapeText(id++, x + 500000, y + 270000, 1200000, 240000, std::to_wstring(values[i]) + L" | " + std::to_wstring(pct) + L"%", 1050, "FFFFFF", true);
    }

    int maxVal = std::max(1, std::max(std::max(values[0], values[1]), std::max(values[2], values[3])));
    ss << ShapeText(id++, 5920000, 3930000, 2500000, 310000, Tr(L"Statistics chart", L"\u7edf\u8ba1\u56fe\u8868"), 1200, "FFFFFF", true);
    for (int i = 0; i < 4; ++i) {
        int y = 4300000 + i * 280000;
        ss << ShapeText(id++, 5920000, y - 50000, 950000, 240000, Tr(labels[i], labels[i]), 780, "B8B8B8", true);
        ss << RectShape(id++, 6920000, y, 3000000, 120000, "242424", "242424", 100000);
        const int barWidth = static_cast<int>(int64_t{values[i]} * 3000000 / maxVal) + 1000;
        ss << RectShape(id++, 6920000, y, barWidth, 120000, colors[i], colors[i], 100000);
        ss << ShapeText(id++, 10120000, y - 60000, 600000, 230000, std::to_wstring(values[i]), 780, "FFFFFF", true);
    }
    return SlideXml(ss.str());
}

std::string BuildPptSlide3(const std::vector<AttendanceRecord>& records){
    int id = 3;
    std::vector<std::pair<std::wstring, PptStats>> days;
    std::unordered_map<std::wstring, size_t> dayIndices;
    for (const auto& r : records) {
        if (!RecordMatchesStatsRange(r)) continue;
        std::wstring day = r.dateTime.size() >= 10 ? r.dateTime.substr(0, 10) : r.dateTime;
        const auto index = dayIndices.emplace(day, days.size());
        if (index.second) days.push_back({std::move(day), {}});
        auto& dayStats = days[index.first->second].second;
        ++dayStats.total;
        if (r.status == L"Present") ++dayStats.present;
        else if (r.status == L"Absent") ++dayStats.absent;
        else if (r.status == L"Late") ++dayStats.late;
        else ++dayStats.other;
    }
    std::sort(days.begin(), days.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    if (days.size() > 8) days.erase(days.begin(), days.end() - 8);

    std::ostringstream ss;
    ss << SlideBackground();
    ss << ShapeText(id++, 760000, 520000, 8800000, 520000, Tr(L"Attendance", L"\u51fa\u52e4\u7387") + L" " + Tr(L"Statistics chart", L"\u7edf\u8ba1\u56fe\u8868"), 2800, "FFFFFF", true);
    ss << ShapeText(id++, 780000, 1050000, 7200000, 300000, Tr(L"Attendance", L"\u51fa\u52e4\u7387") + L" trend by date", 1150, "B8B8B8");
    ss << RectShape(id++, 760000, 1420000, 10670000, 22000, "BDBDBD", "BDBDBD", 100000);
    const wchar_t* labels[] = {L"Present", L"Absent", L"Late", L"Other"};
    const char* colors[] = {"8CA39A", "A67575", "A89876", "9A9A9A"};

    double latestRate = 0.0;
    if (!days.empty()) {
        const auto& latest = days.back().second;
        latestRate = latest.total ? latest.present * 100.0 / latest.total : 0.0;
    }
    std::wstringstream pct;
    pct.setf(std::ios::fixed);
    pct.precision(0);
    pct << latestRate << L"%";

    ss << RectShape(id++, 9000000, 520000, 2140000, 700000, "121212", "444444", 93000);
    ss << ShapeText(id++, 9250000, 650000, 1100000, 260000, Tr(L"Attendance", L"\u51fa\u52e4\u7387"), 900, "8CA39A", true);
    ss << ShapeText(id++, 10200000, 600000, 800000, 400000, pct.str(), 1800, "FFFFFF", true);

    ss << RectShape(id++, 760000, 1700000, 10670000, 3950000, "121212", "444444", 93000);
    for (int i = 0; i < 4; ++i) {
        int x = 1180000 + i * 1750000;
        ss << RectShape(id++, x, 1950000, 210000, 210000, colors[i], colors[i], 100000);
        ss << ShapeText(id++, x + 280000, 1895000, 1180000, 300000, Tr(labels[i], labels[i]), 900, "E0E0E0", true);
    }

    int x0 = 1180000, y0 = 5300000, w = 9650000, h = 2800000;
    ss << RectShape(id++, x0, y0, w, 18000, "BDBDBD", "BDBDBD", 100000);
    ss << RectShape(id++, x0, y0 - h, 18000, h, "BDBDBD", "BDBDBD", 100000);
    ss << LineShape(id++, x0, y0 - h / 2, x0 + w, y0 - h / 2, "444444", 14000);
    if (days.empty()) {
        ss << ShapeText(id++, x0 + 600000, y0 - 1900000, 6000000, 500000, Tr(L"There are no records to export.", L"\u6ca1\u6709\u53ef\u5bfc\u51fa\u7684\u8bb0\u5f55\u3002"), 1800, "FFFFFF", true);
    } else {
        int slot = w / std::max(1, (int)days.size());
        int barW = std::clamp(slot / 8, 90000, 210000);
        int gap = std::max(25000, barW / 4);
        int maxCount = 1;
        for (const auto& day : days) {
            maxCount = std::max(maxCount, std::max(std::max(day.second.present, day.second.absent), std::max(day.second.late, day.second.other)));
        }
        std::vector<POINT> trend;
        for (int i = 0; i < (int)days.size(); ++i) {
            int groupW = barW * 4 + gap * 3;
            int groupX = x0 + i * slot + (slot - groupW) / 2;
            int counts[] = {days[i].second.present, days[i].second.absent, days[i].second.late, days[i].second.other};
            for (int j = 0; j < 4; ++j) {
                int barH = counts[j] == 0 ? 45000 : std::max(80000, counts[j] * h / maxCount);
                int x = groupX + j * (barW + gap);
                int y = y0 - barH;
                ss << RectShape(id++, x, y, barW, barH, colors[j], colors[j], 100000);
            }
            double rate = days[i].second.total ? days[i].second.present * 1.0 / days[i].second.total : 0.0;
            int tx = x0 + i * slot + slot / 2;
            int ty = y0 - (int)(rate * h);
            trend.push_back({tx, ty});
            ss << ShapeText(id++, x0 + i * slot, y0 + 110000, slot, 260000, days[i].first, 650, "E0E0E0");
        }
        for (int i = 1; i < (int)trend.size(); ++i) {
            ss << LineShape(id++, trend[i - 1].x, trend[i - 1].y, trend[i].x, trend[i].y, "FFFFFF", 30000);
        }
    }
    return SlideXml(ss.str());
}

std::string PptContentTypes(){
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
           "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
           "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
           "<Override PartName=\"/ppt/presentation.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml\"/>"
           "<Override PartName=\"/ppt/slides/slide1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>"
           "<Override PartName=\"/ppt/slides/slide2.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>"
           "<Override PartName=\"/ppt/slides/slide3.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>"
           "<Override PartName=\"/ppt/presProps.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presProps+xml\"/>"
           "<Override PartName=\"/ppt/viewProps.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.viewProps+xml\"/>"
           "<Override PartName=\"/ppt/tableStyles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.tableStyles+xml\"/>"
           "<Override PartName=\"/ppt/slideMasters/slideMaster1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml\"/>"
           "<Override PartName=\"/ppt/slideLayouts/slideLayout1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml\"/>"
           "<Override PartName=\"/ppt/theme/theme1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/>"
           "<Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>"
           "<Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>"
           "</Types>";
}

std::string PptDefaultTextStyleXml(){
    return "<p:defaultTextStyle><a:defPPr><a:defRPr lang=\"en-US\"/></a:defPPr>"
           "<a:lvl1pPr marL=\"0\" algn=\"l\" defTabSz=\"914400\" rtl=\"0\" eaLnBrk=\"1\" latinLnBrk=\"0\" hangingPunct=\"1\"><a:defRPr sz=\"1800\" kern=\"1200\"><a:solidFill><a:schemeClr val=\"tx1\"/></a:solidFill><a:latin typeface=\"+mn-lt\"/><a:ea typeface=\"+mn-ea\"/><a:cs typeface=\"+mn-cs\"/></a:defRPr></a:lvl1pPr>"
           "<a:lvl2pPr marL=\"457200\" algn=\"l\" defTabSz=\"914400\" rtl=\"0\" eaLnBrk=\"1\" latinLnBrk=\"0\" hangingPunct=\"1\"><a:defRPr sz=\"1800\" kern=\"1200\"><a:solidFill><a:schemeClr val=\"tx1\"/></a:solidFill><a:latin typeface=\"+mn-lt\"/><a:ea typeface=\"+mn-ea\"/><a:cs typeface=\"+mn-cs\"/></a:defRPr></a:lvl2pPr>"
           "<a:lvl3pPr marL=\"914400\" algn=\"l\" defTabSz=\"914400\" rtl=\"0\" eaLnBrk=\"1\" latinLnBrk=\"0\" hangingPunct=\"1\"><a:defRPr sz=\"1800\" kern=\"1200\"><a:solidFill><a:schemeClr val=\"tx1\"/></a:solidFill><a:latin typeface=\"+mn-lt\"/><a:ea typeface=\"+mn-ea\"/><a:cs typeface=\"+mn-cs\"/></a:defRPr></a:lvl3pPr>"
           "<a:lvl4pPr marL=\"1371600\" algn=\"l\" defTabSz=\"914400\" rtl=\"0\" eaLnBrk=\"1\" latinLnBrk=\"0\" hangingPunct=\"1\"><a:defRPr sz=\"1800\" kern=\"1200\"><a:solidFill><a:schemeClr val=\"tx1\"/></a:solidFill><a:latin typeface=\"+mn-lt\"/><a:ea typeface=\"+mn-ea\"/><a:cs typeface=\"+mn-cs\"/></a:defRPr></a:lvl4pPr>"
           "<a:lvl5pPr marL=\"1828800\" algn=\"l\" defTabSz=\"914400\" rtl=\"0\" eaLnBrk=\"1\" latinLnBrk=\"0\" hangingPunct=\"1\"><a:defRPr sz=\"1800\" kern=\"1200\"><a:solidFill><a:schemeClr val=\"tx1\"/></a:solidFill><a:latin typeface=\"+mn-lt\"/><a:ea typeface=\"+mn-ea\"/><a:cs typeface=\"+mn-cs\"/></a:defRPr></a:lvl5pPr>"
           "</p:defaultTextStyle>";
}

std::string PptPresPropsXml(){
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<p:presentationPr xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"/>";
}

std::string PptViewPropsXml(){
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<p:viewPr xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"><p:normalViewPr><p:restoredLeft sz=\"15620\"/><p:restoredTop sz=\"94660\"/></p:normalViewPr><p:slideViewPr><p:cSldViewPr snapToGrid=\"0\"><p:cViewPr varScale=\"1\"><p:scale><a:sx n=\"70\" d=\"100\"/><a:sy n=\"70\" d=\"100\"/></p:scale><p:origin x=\"0\" y=\"0\"/></p:cViewPr><p:guideLst/></p:cSldViewPr></p:slideViewPr><p:notesTextViewPr><p:cViewPr><p:scale><a:sx n=\"1\" d=\"1\"/><a:sy n=\"1\" d=\"1\"/></p:scale><p:origin x=\"0\" y=\"0\"/></p:cViewPr></p:notesTextViewPr><p:gridSpacing cx=\"72008\" cy=\"72008\"/></p:viewPr>";
}

std::string PptTableStylesXml(){
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<a:tblStyleLst xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" def=\"{5C22544A-7EE6-4342-B048-85BDC9FD1C3A}\"/>";
}

std::string PptThemeXml(){
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<a:theme xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" name=\"AttendanceApp\">"
           "<a:themeElements><a:clrScheme name=\"AttendanceApp\"><a:dk1><a:srgbClr val=\"000000\"/></a:dk1><a:lt1><a:srgbClr val=\"FFFFFF\"/></a:lt1><a:dk2><a:srgbClr val=\"121212\"/></a:dk2><a:lt2><a:srgbClr val=\"E0E0E0\"/></a:lt2><a:accent1><a:srgbClr val=\"8CA39A\"/></a:accent1><a:accent2><a:srgbClr val=\"A67575\"/></a:accent2><a:accent3><a:srgbClr val=\"A89876\"/></a:accent3><a:accent4><a:srgbClr val=\"9A9A9A\"/></a:accent4><a:accent5><a:srgbClr val=\"B8B8B8\"/></a:accent5><a:accent6><a:srgbClr val=\"FFFFFF\"/></a:accent6><a:hlink><a:srgbClr val=\"E0E0E0\"/></a:hlink><a:folHlink><a:srgbClr val=\"B8B8B8\"/></a:folHlink></a:clrScheme>"
           "<a:fontScheme name=\"AttendanceApp\"><a:majorFont><a:latin typeface=\"Segoe UI\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:majorFont><a:minorFont><a:latin typeface=\"Segoe UI\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:minorFont></a:fontScheme>"
           "<a:fmtScheme name=\"AttendanceApp\"><a:fillStyleLst>"
           "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>"
           "<a:gradFill rotWithShape=\"1\"><a:gsLst><a:gs pos=\"0\"><a:schemeClr val=\"phClr\"><a:lumMod val=\"110000\"/><a:satMod val=\"105000\"/></a:schemeClr></a:gs><a:gs pos=\"100000\"><a:schemeClr val=\"phClr\"><a:lumMod val=\"90000\"/><a:satMod val=\"105000\"/></a:schemeClr></a:gs></a:gsLst><a:lin ang=\"5400000\" scaled=\"0\"/></a:gradFill>"
           "<a:gradFill rotWithShape=\"1\"><a:gsLst><a:gs pos=\"0\"><a:schemeClr val=\"phClr\"><a:lumMod val=\"105000\"/></a:schemeClr></a:gs><a:gs pos=\"100000\"><a:schemeClr val=\"phClr\"><a:lumMod val=\"75000\"/></a:schemeClr></a:gs></a:gsLst><a:lin ang=\"5400000\" scaled=\"0\"/></a:gradFill>"
           "</a:fillStyleLst><a:lnStyleLst>"
           "<a:ln w=\"9525\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:prstDash val=\"solid\"/></a:ln>"
           "<a:ln w=\"25400\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:prstDash val=\"solid\"/></a:ln>"
           "<a:ln w=\"38100\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:prstDash val=\"solid\"/></a:ln>"
           "</a:lnStyleLst><a:effectStyleLst><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle></a:effectStyleLst>"
           "<a:bgFillStyleLst><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"><a:tint val=\"95000\"/></a:schemeClr></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"><a:shade val=\"85000\"/></a:schemeClr></a:solidFill></a:bgFillStyleLst>"
           "</a:fmtScheme></a:themeElements></a:theme>";
}

std::string PptSlideLayoutXml(){
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<p:sldLayout xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" type=\"blank\" preserve=\"1\"><p:cSld name=\"Blank\"><p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sldLayout>";
}

std::string PptSlideMasterXml(){
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<p:sldMaster xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"><p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld><p:clrMap bg1=\"lt1\" tx1=\"dk1\" bg2=\"lt2\" tx2=\"dk2\" accent1=\"accent1\" accent2=\"accent2\" accent3=\"accent3\" accent4=\"accent4\" accent5=\"accent5\" accent6=\"accent6\" hlink=\"hlink\" folHlink=\"folHlink\"/><p:sldLayoutIdLst><p:sldLayoutId id=\"2147483649\" r:id=\"rId1\"/></p:sldLayoutIdLst><p:txStyles><p:titleStyle/><p:bodyStyle/><p:otherStyle/></p:txStyles></p:sldMaster>";
}

std::vector<ZipEntry> BuildPptxEntries(){
    SyncActiveSheet();
    EnsureSheets();
    const auto& sheet = Document().sheets[Document().activeSheet];
    PptStats stats = CountRecords(sheet.records);
    std::vector<ZipEntry> entries;
    entries.push_back({"[Content_Types].xml", PptContentTypes()});
    entries.push_back({"_rels/.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"ppt/presentation.xml\"/><Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/><Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/></Relationships>"});
    entries.push_back({"docProps/app.xml", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\" xmlns:vt=\"http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes\"><Application>AttendanceApp</Application><PresentationFormat>Widescreen</PresentationFormat><Slides>3</Slides></Properties>"});
    entries.push_back({"docProps/core.xml", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:dcterms=\"http://purl.org/dc/terms/\" xmlns:dcmitype=\"http://purl.org/dc/dcmitype/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"><dc:title>Attendance Report</dc:title><dc:creator>AttendanceApp</dc:creator></cp:coreProperties>"});
    entries.push_back({"ppt/presentation.xml", std::string("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><p:presentation xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" saveSubsetFonts=\"1\"><p:sldMasterIdLst><p:sldMasterId id=\"2147483648\" r:id=\"rId4\"/></p:sldMasterIdLst><p:sldIdLst><p:sldId id=\"256\" r:id=\"rId1\"/><p:sldId id=\"257\" r:id=\"rId2\"/><p:sldId id=\"258\" r:id=\"rId3\"/></p:sldIdLst><p:sldSz cx=\"12192000\" cy=\"6858000\"/><p:notesSz cx=\"6858000\" cy=\"9144000\"/>") + PptDefaultTextStyleXml() + "</p:presentation>"});
    entries.push_back({"ppt/_rels/presentation.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" Target=\"slides/slide1.xml\"/><Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" Target=\"slides/slide2.xml\"/><Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" Target=\"slides/slide3.xml\"/><Relationship Id=\"rId4\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\" Target=\"slideMasters/slideMaster1.xml\"/><Relationship Id=\"rId5\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/presProps\" Target=\"presProps.xml\"/><Relationship Id=\"rId6\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/viewProps\" Target=\"viewProps.xml\"/><Relationship Id=\"rId7\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/tableStyles\" Target=\"tableStyles.xml\"/><Relationship Id=\"rId8\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\" Target=\"theme/theme1.xml\"/></Relationships>"});
    entries.push_back({"ppt/presProps.xml", PptPresPropsXml()});
    entries.push_back({"ppt/viewProps.xml", PptViewPropsXml()});
    entries.push_back({"ppt/tableStyles.xml", PptTableStylesXml()});
    entries.push_back({"ppt/theme/theme1.xml", PptThemeXml()});
    entries.push_back({"ppt/slideMasters/slideMaster1.xml", PptSlideMasterXml()});
    entries.push_back({"ppt/slideMasters/_rels/slideMaster1.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/><Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\" Target=\"../theme/theme1.xml\"/></Relationships>"});
    entries.push_back({"ppt/slideLayouts/slideLayout1.xml", PptSlideLayoutXml()});
    entries.push_back({"ppt/slideLayouts/_rels/slideLayout1.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\" Target=\"../slideMasters/slideMaster1.xml\"/></Relationships>"});
    entries.push_back({"ppt/slides/slide1.xml", BuildPptSlide1(sheet, stats)});
    entries.push_back({"ppt/slides/slide2.xml", BuildPptSlide2(stats)});
    entries.push_back({"ppt/slides/slide3.xml", BuildPptSlide3(sheet.records)});
    entries.push_back({"ppt/slides/_rels/slide1.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/></Relationships>"});
    entries.push_back({"ppt/slides/_rels/slide2.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/></Relationships>"});
    entries.push_back({"ppt/slides/_rels/slide3.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/></Relationships>"});
    return entries;
}

bool ExportPptxFile(const std::wstring& path){
    return WriteZipStore(std::filesystem::path(path), BuildPptxEntries());
}

std::function<bool()> PreparePptxExport(const std::wstring& path) {
    return [path, entries = BuildPptxEntries()]() mutable {
        return WriteZipStore(std::filesystem::path(path), std::move(entries));
    };
}

void ExportPptx(){
    if (Document().Records().empty()) {
        ShowMessage(Tr(L"There are no records to export.", L"\u6ca1\u6709\u53ef\u5bfc\u51fa\u7684\u8bb0\u5f55\u3002"));
        return;
    }
    std::wstring path = SavePptxFileDialog();
    if (path.empty()) return;
    auto complete = [](bool exported) {
        ShowMessage(exported
            ? Tr(L"PowerPoint exported successfully.", L"PowerPoint \u5bfc\u51fa\u6210\u529f\u3002")
            : Tr(L"Could not export the PowerPoint file.", L"\u65e0\u6cd5\u5bfc\u51fa PowerPoint \u6587\u4ef6\u3002"));
    };
    if (!QueuePreparedReport(Ui().hwnd, [path] { return PreparePptxExport(path); }, complete)) complete(false);
}

} // namespace attendance
