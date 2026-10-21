#include "core/status.h"
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


int PptStatusBarWidth(int count,int maximum){return static_cast<int>(int64_t{std::clamp(count,0,std::max(1,maximum))}*6000000/std::max(1,maximum));}
std::string BuildPptSlide1(const AttendanceSheet& sheet,const PptStats& stats){
 int id=3;std::ostringstream ss;ss<<SlideBackground();
 ss<<ShapeText(id++,760000,450000,10600000,500000,Tr(L"Attendance Summary",L"出勤摘要"),2900,"FFFFFF",true);
 ss<<ShapeText(id++,760000,1100000,10600000,450000,sheet.name+L" | "+StatsRangeName(Document().statsRange)+L" | "+ReportTemplateName(Settings().reportTemplate),1400,"BDBDBD");
 ss<<ShapeText(id++,760000,1750000,10600000,700000,Tr(L"Attendance",L"出勤率")+L"  "+AttendanceRateText(stats),3200,"8CA39A",true);
 int values[]={stats.present,stats.absent,stats.late,stats.other,stats.leave,stats.pending};
 for(int i=0;i<6;++i){int x=760000+(i%3)*3580000,y=2800000+(i/3)*1250000;
  ss<<RectShape(id++,x,y,3300000,1000000,"151515","444444");
  ss<<ShapeText(id++,x+220000,y+150000,2800000,300000,StatusDisplayText(StatusKeys[i]),1250,"BDBDBD",true);
  ss<<ShapeText(id++,x+220000,y+460000,2800000,400000,std::to_wstring(values[i]),2500,"FFFFFF",true);
 }
 std::wstring footer;
 if(Settings().reportTemplate==ReportTemplate::Teacher||Settings().reportTemplate==ReportTemplate::Complete)footer=sheet.teacher+L" | "+sheet.location+L" | "+sheet.notes;
 else if(Settings().reportTemplate==ReportTemplate::Parent)footer=Tr(L"Roster students",L"名单学生")+L": "+std::to_wstring(sheet.students.size());
 ss<<ShapeText(id++,760000,5650000,10600000,650000,footer,1100,"BDBDBD");
 return SlideXml(ss.str());
}
std::string BuildPptSlide2(const PptStats& stats){
 int id=3;std::ostringstream ss;ss<<SlideBackground();
 ss<<ShapeText(id++,760000,500000,10600000,500000,Tr(L"Statistics Chart",L"统计图表"),2900,"FFFFFF",true);
 ss<<ShapeText(id++,760000,1150000,10600000,400000,Tr(L"Attendance",L"出勤率")+L": "+AttendanceRateText(stats),1500,"BDBDBD");
 int values[]={stats.present,stats.absent,stats.late,stats.other,stats.leave,stats.pending};
 const char* colors[]={"8CA39A","A67575","A89876","9A9A9A","7E9CBD","85859C"};
 int maximum=std::max(1,*std::max_element(std::begin(values),std::end(values)));
 for(int i=0;i<6;++i){int y=2100000+i*600000;
  ss<<ShapeText(id++,760000,y,2200000,350000,StatusDisplayText(StatusKeys[i]),1300,"FFFFFF");
  ss<<RectShape(id++,3100000,y+80000,6000000,150000,"242424","242424");
  ss<<RectShape(id++,3100000,y+80000,PptStatusBarWidth(values[i],maximum),150000,colors[i],colors[i]);
  ss<<ShapeText(id++,9500000,y,1600000,350000,std::to_wstring(values[i]),1500,"FFFFFF",true);
 }
 return SlideXml(ss.str());
}
std::string BuildPptSlide3(const std::vector<AttendanceRecord>& records){
 int id=3;std::map<std::wstring,PptStats> aggregated;
 for(const auto& r:records)if(RecordMatchesStatsRange(r))AccumulateStatus(aggregated[r.dateTime.substr(0,10)],r.status);
 std::vector<std::pair<std::wstring,PptStats>> days(aggregated.begin(),aggregated.end());
 if(days.size()>8)days.erase(days.begin(),days.end()-8);
 std::ostringstream ss;ss<<SlideBackground();
 ss<<ShapeText(id++,760000,500000,10600000,500000,Tr(L"Attendance trend",L"出勤趋势"),2900,"FFFFFF",true);
 const char* colors[]={"8CA39A","A67575","A89876","9A9A9A","7E9CBD","85859C"};
 for(int j=0;j<6;++j){int x=760000+j*1770000;ss<<RectShape(id++,x,1300000,120000,120000,colors[j],colors[j]);ss<<ShapeText(id++,x+180000,1230000,1500000,280000,StatusDisplayText(StatusKeys[j]),950,"BDBDBD");}
 if(days.empty()){ss<<ShapeText(id++,760000,2800000,10600000,500000,Tr(L"No data",L"暂无数据"),2300,"FFFFFF");return SlideXml(ss.str());}
 int maxCount=1;for(const auto& day:days){auto s=day.second;maxCount=std::max({maxCount,s.present,s.absent,s.late,s.other,s.leave,s.pending});}
 const int slot=10600000/static_cast<int>(days.size()),base=5400000,height=3000000;
 for(size_t i=0;i<days.size();++i){const auto& s=days[i].second;int counts[]={s.present,s.absent,s.late,s.other,s.leave,s.pending};int x=760000+static_cast<int>(i)*slot;
  int bar=std::max(1,slot/9);
  for(int j=0;j<6;++j){int h=static_cast<int>(int64_t{counts[j]}*height/maxCount);ss<<RectShape(id++,x+j*(bar+bar/5),base-h,bar,h,colors[j],colors[j]);}
  ss<<ShapeText(id++,x,5550000,slot,300000,days[i].first,750,"BDBDBD");
  ss<<ShapeText(id++,x,5900000,slot,300000,AttendanceRateText(s),850,"FFFFFF",true);
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
