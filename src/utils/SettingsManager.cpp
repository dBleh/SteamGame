#include "SettingsManager.h"

SettingsManager::SettingsManager() {
    // Try to load settings from file, if that fails, use defaults
    if (!LoadSettings()) {
        std::cout << "[SETTINGS] Failed to load settings, using defaults" << std::endl;
    }
}

bool SettingsManager::LoadSettings() {
    std::ifstream file(settingsFilePath);
    if (!file.is_open()) {
        std::cout << "[SETTINGS] Settings file not found, using defaults" << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue; // Skip comments and empty lines
        }
        
        size_t delimPos = line.find('=');
        if (delimPos == std::string::npos) {
            continue; // Invalid line format
        }
        
        std::string key = line.substr(0, delimPos);
        std::string value = line.substr(delimPos + 1);
        
        // Remove whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // Process key bindings
        if (key == "moveUp") {
            settings.moveUp = StringToKey(value);
        } else if (key == "moveDown") {
            settings.moveDown = StringToKey(value);
        } else if (key == "moveLeft") {
            settings.moveLeft = StringToKey(value);
        } else if (key == "moveRight") {
            settings.moveRight = StringToKey(value);
        } else if (key == "shoot") {
            settings.shoot = StringToKey(value);
        } else if (key == "showLeaderboard") {
            settings.showLeaderboard = StringToKey(value);
        } else if (key == "showMenu") {
            settings.showMenu = StringToKey(value);
        } else if (key == "toggleGrid") {
            settings.toggleGrid = StringToKey(value);
        } else if (key == "toggleCursorLock") {
            settings.toggleCursorLock = StringToKey(value);
        } else if (key == "showFPS") {
            settings.showFPS = (value == "true" || value == "1");
        } else if (key == "volumeLevel") {
            try {
                settings.volumeLevel = std::stoi(value);
            } catch (...) {
                settings.volumeLevel = 100;
            }
        }
    }
    
    file.close();
    std::cout << "[SETTINGS] Settings loaded successfully" << std::endl;
    return true;
}

bool SettingsManager::SaveSettings() {
    std::ofstream file(settingsFilePath);
    if (!file.is_open()) {
        std::cerr << "[SETTINGS] Failed to open settings file for writing" << std::endl;
        return false;
    }
    
    file << "# Game Settings Configuration\n";
    file << "# Input Bindings\n";
    file << "moveUp=" << KeyToString(settings.moveUp) << "\n";
    file << "moveDown=" << KeyToString(settings.moveDown) << "\n";
    file << "moveLeft=" << KeyToString(settings.moveLeft) << "\n";
    file << "moveRight=" << KeyToString(settings.moveRight) << "\n";
    file << "shoot=" << KeyToString(settings.shoot) << "\n";
    file << "showLeaderboard=" << KeyToString(settings.showLeaderboard) << "\n";
    file << "showMenu=" << KeyToString(settings.showMenu) << "\n";
    file << "toggleGrid=" << KeyToString(settings.toggleGrid) << "\n";
    file << "toggleCursorLock=" << KeyToString(settings.toggleCursorLock) << "\n";
    
    file << "\n# Other Settings\n";
    file << "showFPS=" << (settings.showFPS ? "true" : "false") << "\n";
    file << "volumeLevel=" << settings.volumeLevel << "\n";
    
    file.close();
    std::cout << "[SETTINGS] Settings saved successfully" << std::endl;
    return true;
}

void SettingsManager::SetKeyBinding(const std::string& action, sf::Keyboard::Key key) {
    if (action == "moveUp") {
        settings.moveUp = key;
    } else if (action == "moveDown") {
        settings.moveDown = key;
    } else if (action == "moveLeft") {
        settings.moveLeft = key;
    } else if (action == "moveRight") {
        settings.moveRight = key;
    } else if (action == "shoot") {
        settings.shoot = key;
    } else if (action == "showLeaderboard") {
        settings.showLeaderboard = key;
    } else if (action == "showMenu") {
        settings.showMenu = key;
    } else if (action == "toggleGrid") {
        settings.toggleGrid = key;
    } else if (action == "toggleCursorLock") {
        settings.toggleCursorLock = key;
    }
}

std::string SettingsManager::KeyToString(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Unknown: return "Unknown";
        case sf::Keyboard::A: return "A";
        case sf::Keyboard::B: return "B";
        case sf::Keyboard::C: return "C";
        case sf::Keyboard::D: return "D";
        case sf::Keyboard::E: return "E";
        case sf::Keyboard::F: return "F";
        case sf::Keyboard::G: return "G";
        case sf::Keyboard::H: return "H";
        case sf::Keyboard::I: return "I";
        case sf::Keyboard::J: return "J";
        case sf::Keyboard::K: return "K";
        case sf::Keyboard::L: return "L";
        case sf::Keyboard::M: return "M";
        case sf::Keyboard::N: return "N";
        case sf::Keyboard::O: return "O";
        case sf::Keyboard::P: return "P";
        case sf::Keyboard::Q: return "Q";
        case sf::Keyboard::R: return "R";
        case sf::Keyboard::S: return "S";
        case sf::Keyboard::T: return "T";
        case sf::Keyboard::U: return "U";
        case sf::Keyboard::V: return "V";
        case sf::Keyboard::W: return "W";
        case sf::Keyboard::X: return "X";
        case sf::Keyboard::Y: return "Y";
        case sf::Keyboard::Z: return "Z";
        case sf::Keyboard::Num0: return "Num0";
        case sf::Keyboard::Num1: return "Num1";
        case sf::Keyboard::Num2: return "Num2";
        case sf::Keyboard::Num3: return "Num3";
        case sf::Keyboard::Num4: return "Num4";
        case sf::Keyboard::Num5: return "Num5";
        case sf::Keyboard::Num6: return "Num6";
        case sf::Keyboard::Num7: return "Num7";
        case sf::Keyboard::Num8: return "Num8";
        case sf::Keyboard::Num9: return "Num9";
        case sf::Keyboard::Escape: return "Escape";
        case sf::Keyboard::LControl: return "LControl";
        case sf::Keyboard::LShift: return "LShift";
        case sf::Keyboard::LAlt: return "LAlt";
        case sf::Keyboard::LSystem: return "LSystem";
        case sf::Keyboard::RControl: return "RControl";
        case sf::Keyboard::RShift: return "RShift";
        case sf::Keyboard::RAlt: return "RAlt";
        case sf::Keyboard::RSystem: return "RSystem";
        case sf::Keyboard::Menu: return "Menu";
        case sf::Keyboard::LBracket: return "LBracket";
        case sf::Keyboard::RBracket: return "RBracket";
        case sf::Keyboard::Semicolon: return "Semicolon";
        case sf::Keyboard::Comma: return "Comma";
        case sf::Keyboard::Period: return "Period";
        case sf::Keyboard::Quote: return "Quote";
        case sf::Keyboard::Slash: return "Slash";
        case sf::Keyboard::Backslash: return "Backslash";
        case sf::Keyboard::Tilde: return "Tilde";
        case sf::Keyboard::Equal: return "Equal";
        case sf::Keyboard::Hyphen: return "Hyphen";
        case sf::Keyboard::Space: return "Space";
        case sf::Keyboard::Enter: return "Enter";
        case sf::Keyboard::Backspace: return "Backspace";
        case sf::Keyboard::Tab: return "Tab";
        case sf::Keyboard::PageUp: return "PageUp";
        case sf::Keyboard::PageDown: return "PageDown";
        case sf::Keyboard::End: return "End";
        case sf::Keyboard::Home: return "Home";
        case sf::Keyboard::Insert: return "Insert";
        case sf::Keyboard::Delete: return "Delete";
        case sf::Keyboard::Add: return "Add";
        case sf::Keyboard::Subtract: return "Subtract";
        case sf::Keyboard::Multiply: return "Multiply";
        case sf::Keyboard::Divide: return "Divide";
        case sf::Keyboard::Left: return "Left";
        case sf::Keyboard::Right: return "Right";
        case sf::Keyboard::Up: return "Up";
        case sf::Keyboard::Down: return "Down";
        case sf::Keyboard::Numpad0: return "Numpad0";
        case sf::Keyboard::Numpad1: return "Numpad1";
        case sf::Keyboard::Numpad2: return "Numpad2";
        case sf::Keyboard::Numpad3: return "Numpad3";
        case sf::Keyboard::Numpad4: return "Numpad4";
        case sf::Keyboard::Numpad5: return "Numpad5";
        case sf::Keyboard::Numpad6: return "Numpad6";
        case sf::Keyboard::Numpad7: return "Numpad7";
        case sf::Keyboard::Numpad8: return "Numpad8";
        case sf::Keyboard::Numpad9: return "Numpad9";
        case sf::Keyboard::F1: return "F1";
        case sf::Keyboard::F2: return "F2";
        case sf::Keyboard::F3: return "F3";
        case sf::Keyboard::F4: return "F4";
        case sf::Keyboard::F5: return "F5";
        case sf::Keyboard::F6: return "F6";
        case sf::Keyboard::F7: return "F7";
        case sf::Keyboard::F8: return "F8";
        case sf::Keyboard::F9: return "F9";
        case sf::Keyboard::F10: return "F10";
        case sf::Keyboard::F11: return "F11";
        case sf::Keyboard::F12: return "F12";
        case sf::Keyboard::F13: return "F13";
        case sf::Keyboard::F14: return "F14";
        case sf::Keyboard::F15: return "F15";
        case sf::Keyboard::Pause: return "Pause";
        default: return "Unknown";
    }
}

sf::Keyboard::Key SettingsManager::StringToKey(const std::string& keyString) {
    if (keyString == "A") return sf::Keyboard::A;
    if (keyString == "B") return sf::Keyboard::B;
    if (keyString == "C") return sf::Keyboard::C;
    if (keyString == "D") return sf::Keyboard::D;
    if (keyString == "E") return sf::Keyboard::E;
    if (keyString == "F") return sf::Keyboard::F;
    if (keyString == "G") return sf::Keyboard::G;
    if (keyString == "H") return sf::Keyboard::H;
    if (keyString == "I") return sf::Keyboard::I;
    if (keyString == "J") return sf::Keyboard::J;
    if (keyString == "K") return sf::Keyboard::K;
    if (keyString == "L") return sf::Keyboard::L;
    if (keyString == "M") return sf::Keyboard::M;
    if (keyString == "N") return sf::Keyboard::N;
    if (keyString == "O") return sf::Keyboard::O;
    if (keyString == "P") return sf::Keyboard::P;
    if (keyString == "Q") return sf::Keyboard::Q;
    if (keyString == "R") return sf::Keyboard::R;
    if (keyString == "S") return sf::Keyboard::S;
    if (keyString == "T") return sf::Keyboard::T;
    if (keyString == "U") return sf::Keyboard::U;
    if (keyString == "V") return sf::Keyboard::V;
    if (keyString == "W") return sf::Keyboard::W;
    if (keyString == "X") return sf::Keyboard::X;
    if (keyString == "Y") return sf::Keyboard::Y;
    if (keyString == "Z") return sf::Keyboard::Z;
    if (keyString == "Num0") return sf::Keyboard::Num0;
    if (keyString == "Num1") return sf::Keyboard::Num1;
    if (keyString == "Num2") return sf::Keyboard::Num2;
    if (keyString == "Num3") return sf::Keyboard::Num3;
    if (keyString == "Num4") return sf::Keyboard::Num4;
    if (keyString == "Num5") return sf::Keyboard::Num5;
    if (keyString == "Num6") return sf::Keyboard::Num6;
    if (keyString == "Num7") return sf::Keyboard::Num7;
    if (keyString == "Num8") return sf::Keyboard::Num8;
    if (keyString == "Num9") return sf::Keyboard::Num9;
    if (keyString == "Escape") return sf::Keyboard::Escape;
    if (keyString == "LControl") return sf::Keyboard::LControl;
    if (keyString == "LShift") return sf::Keyboard::LShift;
    if (keyString == "LAlt") return sf::Keyboard::LAlt;
    if (keyString == "LSystem") return sf::Keyboard::LSystem;
    if (keyString == "RControl") return sf::Keyboard::RControl;
    if (keyString == "RShift") return sf::Keyboard::RShift;
    if (keyString == "RAlt") return sf::Keyboard::RAlt;
    if (keyString == "RSystem") return sf::Keyboard::RSystem;
    if (keyString == "Menu") return sf::Keyboard::Menu;
    if (keyString == "LBracket") return sf::Keyboard::LBracket;
    if (keyString == "RBracket") return sf::Keyboard::RBracket;
    if (keyString == "Semicolon") return sf::Keyboard::Semicolon;
    if (keyString == "Comma") return sf::Keyboard::Comma;
    if (keyString == "Period") return sf::Keyboard::Period;
    if (keyString == "Quote") return sf::Keyboard::Quote;
    if (keyString == "Slash") return sf::Keyboard::Slash;
    if (keyString == "Backslash") return sf::Keyboard::Backslash;
    if (keyString == "Tilde") return sf::Keyboard::Tilde;
    if (keyString == "Equal") return sf::Keyboard::Equal;
    if (keyString == "Hyphen") return sf::Keyboard::Hyphen;
    if (keyString == "Space") return sf::Keyboard::Space;
    if (keyString == "Enter") return sf::Keyboard::Enter;
    if (keyString == "Backspace") return sf::Keyboard::Backspace;
    if (keyString == "Tab") return sf::Keyboard::Tab;
    if (keyString == "PageUp") return sf::Keyboard::PageUp;
    if (keyString == "PageDown") return sf::Keyboard::PageDown;
    if (keyString == "End") return sf::Keyboard::End;
    if (keyString == "Home") return sf::Keyboard::Home;
    if (keyString == "Insert") return sf::Keyboard::Insert;
    if (keyString == "Delete") return sf::Keyboard::Delete;
    if (keyString == "Add") return sf::Keyboard::Add;
    if (keyString == "Subtract") return sf::Keyboard::Subtract;
    if (keyString == "Multiply") return sf::Keyboard::Multiply;
    if (keyString == "Divide") return sf::Keyboard::Divide;
    if (keyString == "Left") return sf::Keyboard::Left;
    if (keyString == "Right") return sf::Keyboard::Right;
    if (keyString == "Up") return sf::Keyboard::Up;
    if (keyString == "Down") return sf::Keyboard::Down;
    if (keyString == "Numpad0") return sf::Keyboard::Numpad0;
    if (keyString == "Numpad1") return sf::Keyboard::Numpad1;
    if (keyString == "Numpad2") return sf::Keyboard::Numpad2;
    if (keyString == "Numpad3") return sf::Keyboard::Numpad3;
    if (keyString == "Numpad4") return sf::Keyboard::Numpad4;
    if (keyString == "Numpad5") return sf::Keyboard::Numpad5;
    if (keyString == "Numpad6") return sf::Keyboard::Numpad6;
    if (keyString == "Numpad7") return sf::Keyboard::Numpad7;
    if (keyString == "Numpad8") return sf::Keyboard::Numpad8;
    if (keyString == "Numpad9") return sf::Keyboard::Numpad9;
    if (keyString == "F1") return sf::Keyboard::F1;
    if (keyString == "F2") return sf::Keyboard::F2;
    if (keyString == "F3") return sf::Keyboard::F3;
    if (keyString == "F4") return sf::Keyboard::F4;
    if (keyString == "F5") return sf::Keyboard::F5;
    if (keyString == "F6") return sf::Keyboard::F6;
    if (keyString == "F7") return sf::Keyboard::F7;
    if (keyString == "F8") return sf::Keyboard::F8;
    if (keyString == "F9") return sf::Keyboard::F9;
    if (keyString == "F10") return sf::Keyboard::F10;
    if (keyString == "F11") return sf::Keyboard::F11;
    if (keyString == "F12") return sf::Keyboard::F12;
    if (keyString == "F13") return sf::Keyboard::F13;
    if (keyString == "F14") return sf::Keyboard::F14;
    if (keyString == "F15") return sf::Keyboard::F15;
    if (keyString == "Pause") return sf::Keyboard::Pause;
    if (keyString.substr(0, 5) == "Mouse") {
        return sf::Keyboard::Unknown;
    }
    
    return sf::Keyboard::Unknown;
}