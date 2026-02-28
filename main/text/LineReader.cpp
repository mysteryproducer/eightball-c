#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include "esp_log.h"
#include "esp_system.h"

#include "gen.h"
#include "files.h"

using namespace EightBall;
using namespace std;

static const char *TAG = "8 ball file line generator";

LineReader::LineReader(const char *file) {
    this->filename = strdup(file);
    this->readOffsets();
}

string LineReader::generateNext() {
    if (lineOffsets.empty()) {
        return "";
    }
    size_t index = rand() % lineOffsets.size();
    line_bounds bounds = lineOffsets[index];
    init_filesystem();
    FILE *f = fopen(this->filename, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", this->filename);
        return "";
    }
    fseek(f, bounds.from, SEEK_SET);
    size_t bytesToRead = bounds.to - bounds.from;
    char buffer[bytesToRead + 1]; // +1 for null terminator
    size_t bytesRead = fread(buffer, 1, bytesToRead, f);
    buffer[bytesRead] = '\0';
    fclose(f);
    close_filesystem();
    return string(buffer);
}

void LineReader::readOffsets() {
    init_filesystem();
    FILE *f = fopen(this->filename, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", this->filename);
        return;
    }
    uint32_t last_offset = 0;
    uint32_t offset = 0;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), f)) {
        offset = ftell(f);
        if (buffer[0] == '\n' || buffer[0] == '\r' || strlen(buffer) == 0) {
            last_offset = offset;
            continue; // skip empty lines
        }
        lineOffsets.push_back({.from=last_offset, .to=offset});
        last_offset = offset;
    }
    fclose(f);
    close_filesystem();
}
