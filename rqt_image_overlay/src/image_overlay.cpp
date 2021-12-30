// Copyright 2021 Kenji Brameld
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <functional>
#include <string>
#include "./image_overlay.hpp"
#include "pluginlib/class_list_macros.hpp"

namespace rqt_image_overlay
{

ImageOverlay::ImageOverlay()
: rqt_gui_cpp::Plugin()
{
}

void ImageOverlay::initPlugin(qt_gui_cpp::PluginContext & context)
{
  widget = new QWidget();
  ui.setupUi(widget);
  context.addWidget(widget);

  menu = new QMenu(widget);
  imageManager = new ImageManager(widget, node_);
  overlayManager = new OverlayManager(widget, node_);
  compositor = new Compositor(widget, *imageManager, *overlayManager, 30.0);

  ui.overlay_table->setModel(overlayManager);
  ui.image_topics_combo_box->setModel(imageManager);

  fillOverlayMenu();

  ui.image_topics_combo_box->setCurrentIndex(ui.image_topics_combo_box->findText(""));
  connect(
    ui.image_topics_combo_box, SIGNAL(currentTextChanged(QString)), imageManager,
    SLOT(onTopicChanged(QString)));

  connect(
    ui.refresh_image_topics_button, SIGNAL(pressed()), imageManager,
    SLOT(updateImageTopicList()));

  connect(ui.remove_overlay_button, SIGNAL(pressed()), this, SLOT(removeOverlay()));

  compositor->setCallableSetImage(
    std::bind(
      &CompositionFrame::setImage, ui.image_frame,
      std::placeholders::_1));
}

void ImageOverlay::shutdownPlugin()
{
  imageManager->shutdownSubscription();
  overlayManager->shutdownSubscriptions();
  overlayManager->shutdownTimer();
  compositor->shutdownTimer();
}

void ImageOverlay::removeOverlay()
{
  QItemSelectionModel * select = ui.overlay_table->selectionModel();
  if (select) {
    for (auto const & index : select->selectedRows()) {
      overlayManager->removeOverlay(index.row());
    }
  }
}


void ImageOverlay::saveSettings(
  qt_gui_cpp::Settings &,
  qt_gui_cpp::Settings & instance_settings) const
{
  instance_settings.setValue("image_topic", ui.image_topics_combo_box->currentText());
  overlayManager->saveSettings(instance_settings);
}

void ImageOverlay::restoreSettings(
  const qt_gui_cpp::Settings &,
  const qt_gui_cpp::Settings & instance_settings)
{
  if (instance_settings.contains("image_topic")) {
    QString topic = instance_settings.value("image_topic").toString();
    if (topic != "") {
      imageManager->setTopicExplicitly(topic);
      ui.image_topics_combo_box->setCurrentIndex(1);
    }
  }

  overlayManager->restoreSettings(instance_settings);
}

void ImageOverlay::fillOverlayMenu()
{
  menu->clear();

  for (const std::string & str_plugin_class : overlayManager->getDeclaredPluginClasses()) {
    QString qstr_plugin_class = QString::fromStdString(str_plugin_class);
    QAction * action = new QAction(qstr_plugin_class, this);
    menu->addAction(action);  // ownership transferred
    connect(
      action, &QAction::triggered, [this, str_plugin_class] {
        overlayManager->addOverlay(str_plugin_class);
      });
  }

  ui.add_overlay_button->setMenu(menu);
}


}  // namespace rqt_image_overlay


PLUGINLIB_EXPORT_CLASS(rqt_image_overlay::ImageOverlay, rqt_gui_cpp::Plugin)
