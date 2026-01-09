#include "analyzer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <iostream>
#include <string_view>
#include <algorithm>
#include <cctype>
#include <fstream>

using namespace std;

//bosluk kontrol ve trim
static inline bool is_ws(unsigned char ch) {
    return std::isspace(ch);
}

// bosluklari temizle
static inline string_view trim_view(string_view text) {
    size_t start = 0, end = text.size();

    while (start < end && is_ws((unsigned char)text[start])) start++;
    while (end > start && is_ws((unsigned char)text[end - 1])) end--;

    return text.substr(start, end - start);
}


//tarih + saatten saati ayir hatada false
static inline bool parseHourRobust_sv(string_view dateTime, int& hourOut) {

    // saat kismindan onceki ayiriciyi buluyor
    size_t spacePos = dateTime.find(' ');
    size_t tPos = dateTime.find('T');
    size_t separatorPos =
        (spacePos == string_view::npos) ? tPos :
        (tPos == string_view::npos) ? spacePos :
        min(spacePos, tPos);

    if (separatorPos == string_view::npos) return false;
    size_t pos = separatorPos + 1;
    while (pos < dateTime.size() && is_ws((unsigned char)dateTime[pos])) pos++;

    // en az bir rakam olup oladisini kontroio ediyor
    if (pos >= dateTime.size() || !isdigit((unsigned char)dateTime[pos]))
        return false;

    // saat degerini okuyor
    int hour = dateTime[pos++] - '0';
    if (pos < dateTime.size() && isdigit((unsigned char)dateTime[pos]))
        hour = hour * 10 + (dateTime[pos++] - '0');

    // saat araligi kontrolu
    if (hour < 0 || hour > 23) return false;
    hourOut = hour;
    return true;
}


//csv kodunu alan fonskiyonun hackerrank hali
void TripAnalyzer::ingestFile(const std::string& filename) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        return; //dosya acilmazsa cikmak icin
    }

    //eski verileri sifirla
    zoneCounts.clear();
    zoneHourCounts.clear();
    zoneCounts.reserve(1 << 15);
    zoneHourCounts.reserve(1 << 15);

    string currentLine;
    bool firstLine = true;

    while (getline(inputFile, currentLine)) {
        string_view lineView = trim_view(currentLine);
        if (lineView.empty()) continue;

        //headeri atliyoruz
        if (firstLine) {
            firstLine = false;
            continue;
        }

        int columnIndex = 0;
        size_t tokenStart = 0;
        string_view zoneField, dateTimeField;
        bool anyEmpty = false;

        //csv alanlarini ayiriyoruz
        for (size_t i = 0; i <= lineView.size(); ++i) {
            if (i == lineView.size() || lineView[i] == ',') {
                string_view token = trim_view(lineView.substr(tokenStart, i - tokenStart));
                if (token.empty()) {
                    anyEmpty = true;
                    break;
                }

                if (columnIndex == 1) zoneField = token;
                else if (columnIndex == 3) dateTimeField = token;

                columnIndex++;
                tokenStart = i + 1;
            }
        }

        //eksik bozuk satirlari atla
        if (anyEmpty || columnIndex < 6 || zoneField.empty() || dateTimeField.empty())
            continue;

        int hour;
        if (!parseHourRobust_sv(dateTimeField, hour)) continue;

        string zone(zoneField);
        zoneCounts[zone]++;

        //zonr varsa ekliyoruz yoksa sayaci guncelle
        auto [it, inserted] =
            zoneHourCounts.try_emplace(zone, array<long long, 24>{});
        it->second[hour]++;
    }
}

std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    if (k <= 0) return {};

    std::vector<ZoneCount> result;
    result.reserve(zoneCounts.size());

    //mapdeki verileri vectore koyuyoruz ki siralayalim
    for (const auto& entry : zoneCounts)
        result.push_back({ entry.first, entry.second });

    //counta gore sirala esitse id ye gore sirala
    auto cmp = [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
        };

    //k den fazla ise ilk k yi aliiyoruz
    if ((int)result.size() > k) {
        partial_sort(result.begin(), result.begin() + k, result.end(), cmp);
        result.resize(k);
    }
    else {
        sort(result.begin(), result.end(), cmp);
    }

    return result;
}

std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    if (k <= 0) return {};

    std::vector<SlotCount> result;
    result.reserve(zoneHourCounts.size() * 2);

    //her zone icin 24 saati gecip 0 olanlari atliyoruz
    for (const auto& zoneEntry : zoneHourCounts) {
        for (int hour = 0; hour < 24; ++hour) {
            long long count = zoneEntry.second[hour];
            if (count > 0)
                result.push_back({ zoneEntry.first, hour, count });
        }
    }

    //count,zone,hour oncekli siralama
    auto cmp = [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
        };

    if ((int)result.size() > k) {
        partial_sort(result.begin(), result.begin() + k, result.end(), cmp);
        result.resize(k);
    }
    else {
        sort(result.begin(), result.end(), cmp);
    }

    return result;
}
