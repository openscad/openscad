#pragma once

#include <QString>
#include <QStringList>
#include <filesystem>
#include <map>
#include <memory>
#include <boost/property_tree/ptree.hpp>

namespace fs = std::filesystem;

class EditorColorScheme
{
public:
  static std::shared_ptr<EditorColorScheme> load(const fs::path& path);

  EditorColorScheme(QString name, int index, boost::property_tree::ptree pt);
  ~EditorColorScheme() = default;

  const QString& name() const { return name_; }
  int index() const { return index_; }
  const boost::property_tree::ptree& propertyTree() const { return pt_; }

private:
  QString name_;
  int index_;
  boost::property_tree::ptree pt_;
};

class EditorColorMap
{
public:
  using colorscheme_set_t = std::multimap<int, std::shared_ptr<EditorColorScheme>, std::less<>>;

  static EditorColorMap *inst();

  QStringList colorSchemeNames() const;
  std::shared_ptr<EditorColorScheme> getColorScheme(const QString& name) const;

private:
  EditorColorMap();
  void enumerateColorSchemesInPath(colorscheme_set_t& result_set, const fs::path& path);

  colorscheme_set_t color_schemes_;
};
