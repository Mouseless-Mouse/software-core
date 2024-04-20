#include "settings.h"

#include "duktape.h"
#include <string>
#include <unordered_map>

void settings::create_js_hooks(duk_context *ctx) {
    duk_push_global_object(ctx);
    duk_push_object(ctx);
    duk_push_c_function(ctx, _change_setting, 2);
    duk_put_prop_string(ctx, -2, "set");
    duk_push_c_function(ctx, _get_setting, 1);
    duk_put_prop_string(ctx, -2, "get");
    duk_put_prop_string(ctx, -2, "settings");
}

duk_ret_t settings::_change_setting(duk_context *ctx) {
    const char *key = duk_get_string(ctx, 0);
    if (duk_is_string(ctx, 1)) {
        const char *value = duk_get_string(ctx, 1);
        _settings[key] = Setting(new std::string(value));
        return DUK_ERR_NONE;
    } else if (duk_is_number(ctx, 1)) {
        double value = duk_get_number(ctx, 1);
        _settings[key] = Setting(value);
        return DUK_ERR_NONE;
    } else if (duk_is_boolean(ctx, 1)) {
        bool value = duk_get_boolean(ctx, 1);
        _settings[key] = Setting(value);
        return DUK_ERR_NONE;
    }
    return DUK_ERR_RANGE_ERROR;
}

duk_ret_t settings::_get_setting(duk_context *ctx) {
    const char *key = duk_get_string(ctx, 0);
    if (_settings.find(key) == _settings.end()) {
        duk_push_undefined(ctx);
    } else {
        switch (_settings[key].type) {
        case SettingType::STRING:
            duk_push_string(ctx, _settings[key].string->c_str());
            break;
        case SettingType::NUMBER:
            duk_push_number(ctx, _settings[key].number);
            break;
        case SettingType::BOOLEAN:
            duk_push_boolean(ctx, _settings[key].boolean);
            break;
        }
    }
    return 1;
}

void settings::set(std::string key, std::string value) {
    _settings[key] = Setting(new std::string(value));
}

void settings::set(std::string key, double value) {
    _settings[key] = Setting(value);
}

void settings::set(std::string key, bool value) {
    _settings[key] = Setting(value);
}

meta::result_t<std::string *, settings::SettingError>
settings::get_string(std::string key) {
    if (_settings.find(key) != _settings.end() &&
        _settings[key].type == SettingType::STRING) {
        return _settings[key].string;
    }
    return settings::SettingError::NOT_FOUND;
}

meta::result_t<double, settings::SettingError>
settings::get_number(std::string key) {
    if (_settings.find(key) != _settings.end() &&
        _settings[key].type == SettingType::NUMBER) {
        return _settings[key].number;
    }
    return settings::SettingError::NOT_FOUND;
}

meta::result_t<bool, settings::SettingError>
settings::get_boolean(std::string key) {
    if (_settings.find(key) != _settings.end() &&
        _settings[key].type == SettingType::BOOLEAN) {
        return _settings[key].boolean;
    }
    return settings::SettingError::NOT_FOUND;
}