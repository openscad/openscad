#include "gui/EditorColorMap.h"
#include "platform/PlatformUtils.h"
#include "utils/printutils.h"

#include <filesystem>
#include <memory>
#include <utility>
#include <string>
#include <boost/property_tree/json_parser.hpp>

std::shared_ptr<EditorColorScheme> EditorColorScheme::load(const fs::path& path)
{
  boost::property_tree::ptree pt;
  try {
    boost::property_tree::read_json(path.string(), pt);
  } catch (const std::exception& e) {
    LOG("Error reading color scheme file '%1$s': %2$s", path.generic_string(), e.what());
    return nullptr;
  }

  auto name = pt.get<std::string>("name", "");
  if (name.empty()) {
    return nullptr;
  }
  auto index = pt.get<int>("index", 0);

  return std::make_shared<EditorColorScheme>(QString::fromStdString(name), index, std::move(pt));
}

EditorColorScheme::EditorColorScheme(QString name, int index, boost::property_tree::ptree pt)
  : name_(std::move(name)), index_(index), pt_(std::move(pt))
{
}

EditorColorMap *EditorColorMap::inst()
{
  static EditorColorMap instance;
  return &instance;
}

EditorColorMap::EditorColorMap()
{
  enumerateColorSchemesInPath(color_schemes_, PlatformUtils::resourceBasePath());
  enumerateColorSchemesInPath(color_schemes_, PlatformUtils::userConfigPath());
}

void EditorColorMap::enumerateColorSchemesInPath(EditorColorMap::colorscheme_set_t& result_set,
                                                 const fs::path& path)
{
  const auto color_schemes = path / "color-schemes" / "editor";

  if (fs::exists(color_schemes) && fs::is_directory(color_schemes)) {
    for (const auto& dirEntry : fs::directory_iterator(color_schemes)) {
      if (!dirEntry.is_regular_file()) continue;

      const auto& entryPath = dirEntry.path();
      if (entryPath.extension() != ".json") continue;

      auto colorScheme = EditorColorScheme::load(entryPath);
      if (colorScheme) {
        result_set.emplace(colorScheme->index(), std::move(colorScheme));
      }
    }
  }
}

QStringList EditorColorMap::colorSchemeNames() const
{
  QStringList names;
  for (const auto& entry : color_schemes_) {
    names << entry.second->name();
  }
  names << "Off";
  return names;
}

std::shared_ptr<EditorColorScheme> EditorColorMap::getColorScheme(const QString& name) const
{
  for (const auto& entry : color_schemes_) {
    if (entry.second->name() == name) {
      return entry.second;
    }
  }
  return nullptr;
}
