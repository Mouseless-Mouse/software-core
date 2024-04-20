#pragma once

#include "duktape.h"
#include "meta.h"
#include <string>
#include <unordered_map>

namespace settings {

enum class SettingType { STRING, NUMBER, BOOLEAN, UNSET };
struct Setting {
    SettingType type;
    union {
        std::string *string;
        double number;
        bool boolean;
    };

    Setting() : type(SettingType::UNSET) {}
    Setting(std::string *string) : type(SettingType::STRING), string(string) {}
    Setting(double number) : type(SettingType::NUMBER), number(number) {}
    Setting(bool boolean) : type(SettingType::BOOLEAN), boolean(boolean) {}
    ~Setting() {
        if (type == SettingType::STRING) {
            delete string;
        }
    }
};

enum class SettingError { NOT_FOUND, INVALID_TYPE };

static std::unordered_map<std::string, Setting> _settings;

static duk_ret_t _change_setting(duk_context *ctx);
static duk_ret_t _get_setting(duk_context *ctx);

void set(std::string key, std::string value);
void set(std::string key, double value);
void set(std::string key, bool value);
meta::result_t<std::string *, SettingError> get_string(std::string key);
meta::result_t<double, SettingError> get_number(std::string key);
meta::result_t<bool, SettingError> get_boolean(std::string key);

void create_js_hooks(duk_context *ctx);

} // namespace settings