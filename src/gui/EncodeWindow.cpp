/*
 * Copyright (C) 2018 Werner Turing <werner.turing@protonmail.com>
 *
 * This file is part of multi-delogo.
 *
 * multi-delogo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * multi-delogo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with multi-delogo.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <memory>
#include <string>
#include <fstream>

#include <gtkmm.h>
#include <glibmm/i18n.h>

#include "EncodeWindow.hpp"

using namespace mdl;


EncodeWindow::EncodeWindow(std::unique_ptr<fg::FilterData> filter_data)
  : filter_data_(std::move(filter_data))
  , codec_(Codec::H264)
{
  set_title(_("Encode video"));
  set_border_width(8);

  Gtk::Box* vbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 8));

  vbox->pack_start(*create_file_selection(), true, true);
  vbox->pack_start(*create_codec(), true, true);
  vbox->pack_start(*create_quality(), true, true);
  vbox->pack_start(*create_buttons(), true, true);

  add(*vbox);
}


Gtk::Box* EncodeWindow::create_file_selection()
{
  Gtk::Label* lbl = Gtk::manage(new Gtk::Label(_("_Output file:"), true));
  lbl->set_mnemonic_widget(txt_file_);

  Gtk::Button* btn = Gtk::manage(new Gtk::Button(_("_Select"), true));
  btn->set_image_from_icon_name("document-open");
  btn->signal_clicked().connect(sigc::mem_fun(*this, &EncodeWindow::on_select_file));

  Gtk::Box* box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
  box->pack_start(*lbl, false, false);
  box->pack_start(txt_file_, true, true);
  box->pack_start(*btn, false, false);

  return box;
}


void EncodeWindow::on_select_file()
{
  Gtk::FileChooserDialog dlg(*this, _("Select output file"), Gtk::FILE_CHOOSER_ACTION_SAVE);
  dlg.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
  dlg.add_button(_("_Save"), Gtk::RESPONSE_OK);

  if (dlg.run() == Gtk::RESPONSE_OK) {
    txt_file_.set_text(dlg.get_file()->get_path());
  }
}


Gtk::Box* EncodeWindow::create_codec()
{
  Gtk::Label* lbl = Gtk::manage(new Gtk::Label(_("Video format:")));

  Gtk::RadioButton* btn_h264 = Gtk::manage(new Gtk::RadioButton("H.26_4", true));
  btn_h264->set_tooltip_text(_("Most compatible video format. If in doubt, use this format"));
  btn_h264->signal_toggled().connect(
    sigc::bind<Codec>(
      sigc::mem_fun(*this, &EncodeWindow::on_codec),
      Codec::H264));

  Gtk::RadioButton* btn_h265 = Gtk::manage(new Gtk::RadioButton("H.26_5", true));
  btn_h265->join_group(*btn_h264);
  btn_h265->set_tooltip_text(_("A newer format that produces smaller video files. May not work on all players"));
  btn_h265->signal_toggled().connect(
    sigc::bind<Codec>(
      sigc::mem_fun(*this, &EncodeWindow::on_codec),
      Codec::H265));

  Gtk::Box* box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
  box->pack_start(*lbl, false, false);
  box->pack_start(*btn_h264, false, false);
  box->pack_start(*btn_h265, false, false);

  return box;
}


void EncodeWindow::on_codec(Codec codec)
{
  codec_ = codec;
  if (codec_ == Codec::H264) {
    txt_quality_.set_value(H264_DEFAULT_CRF_);
  } else {
    txt_quality_.set_value(H265_DEFAULT_CRF_);
  }
}


Gtk::Box* EncodeWindow::create_quality()
{
  Gtk::Label* lbl = Gtk::manage(new Gtk::Label(_("_Quality:"), true));
  lbl->set_mnemonic_widget(txt_quality_);

  txt_quality_.set_range(0, 51);
  txt_quality_.set_increments(1, 1);
  txt_quality_.set_value(H264_DEFAULT_CRF_);
  txt_quality_.set_tooltip_text(_("CRF value to use for encoding. Lower values generally lead to higher quality, but bigger files. If in doubt, accept the default"));

  Gtk::Box* box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
  box->pack_start(*lbl, false, false);
  box->pack_start(txt_quality_, false, true);

  return box;
}


Gtk::Box* EncodeWindow::create_buttons()
{
  Gtk::Button* btn_script = Gtk::manage(new Gtk::Button(_("_Generate filter script"), true));
  btn_script->set_tooltip_text(_("Generates a ffmpeg filter script file that can be used to encode the video. Use this option if you want to run ffmpeg manually with custom encoding options"));
  btn_script->signal_clicked().connect(sigc::mem_fun(*this, &EncodeWindow::on_generate_script));

  Gtk::Button* btn_encode = Gtk::manage(new Gtk::Button(_("Encode"), true));
  btn_encode->signal_clicked().connect(sigc::mem_fun(*this, &EncodeWindow::on_encode));

  Gtk::Box* box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  box->pack_end(*btn_encode, false, false);
  box->pack_end(*btn_script, false, false);

  return box;
}


void EncodeWindow::on_encode()
{
  std::string file = txt_file_.get_text();
  if (!check_file(file)) {
    return;
  }

  tmp_fd_ = Glib::file_open_tmp(tmp_filter_file_, "mdlfilter");
  generate_script(tmp_filter_file_);

  std::vector<std::string> cmd_line = get_ffmpeg_cmd_line(tmp_filter_file_);

  start_ffmpeg(cmd_line);
}


void EncodeWindow::on_generate_script()
{
  std::string file = txt_file_.get_text();
  if (!check_file(file)) {
    return;
  }

  generate_script(file);

  Gtk::MessageDialog dlg(*this, _("Filter script generated"));
  dlg.run();
}


bool EncodeWindow::check_file(const std::string& file)
{
  if (file.empty()) {
    Gtk::MessageDialog dlg(*this, _("Please select the output file"), false, Gtk::MESSAGE_ERROR);
    dlg.run();
    return false;
  }

  if (file_exists(file)) {
    Gtk::MessageDialog dlg(*this,
                           Glib::ustring::compose(_("File %1 already exists. Overwrite?"), file),
                           false,
                           Gtk::MESSAGE_QUESTION,
                           Gtk::BUTTONS_NONE);
    dlg.add_button(_("_Cancel"), Gtk::RESPONSE_NO);
    dlg.add_button(_("_Overwrite"), Gtk::RESPONSE_YES);
    return dlg.run() == Gtk::RESPONSE_YES;
  }

  return true;
}


bool EncodeWindow::file_exists(const std::string& file)
{
  return Glib::file_test(file, Glib::FILE_TEST_EXISTS);
}


void EncodeWindow::generate_script(const std::string& file)
{
  std::ofstream file_stream(file);
  if (!file_stream.is_open()) {
    auto msg = Glib::ustring::compose(_("Could not open file %1: %2"),
                                      file, Glib::strerror(errno));
    Gtk::MessageDialog dlg(*this, msg, false, Gtk::MESSAGE_ERROR);
    dlg.run();
    return;
  }

  filter_data_->filter_list().generate_ffmpeg_script(file_stream);
  file_stream.close();
}


std::vector<std::string> EncodeWindow::get_ffmpeg_cmd_line(const std::string& filter_file)
{
  std::string codec_name;
  if (codec_ == Codec::H264) {
    codec_name = "libx264";
  } else if (codec_ == Codec::H265) {
    codec_name = "libx265";
  }

  std::string quality = std::to_string(txt_quality_.get_value_as_int());

  std::vector<std::string> cmd_line;
  cmd_line.push_back("ffmpeg");
  cmd_line.push_back("-y");
  cmd_line.push_back("-v"); cmd_line.push_back("quiet");
  cmd_line.push_back("-stats");
  cmd_line.push_back("-i"); cmd_line.push_back(filter_data_->movie_file());
  cmd_line.push_back("-filter_script:v"); cmd_line.push_back(filter_file);
  cmd_line.push_back("-c:v"); cmd_line.push_back(codec_name);
  cmd_line.push_back("-crf"); cmd_line.push_back(quality);
  cmd_line.push_back("-c:a"); cmd_line.push_back("copy");
  cmd_line.push_back(txt_file_.get_text());

  return cmd_line;
}


void EncodeWindow::start_ffmpeg(const std::vector<std::string>& cmd_line)
{
  Glib::Pid ffmpeg_pid;
  int ffmpeg_stderr_fd;
  try {
    Glib::spawn_async_with_pipes("",
                                 cmd_line,
                                 Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDOUT_TO_DEV_NULL,
                                 Glib::SlotSpawnChildSetup(),
                                 &ffmpeg_pid,
                                 nullptr,
                                 nullptr,
                                 &ffmpeg_stderr_fd);
  } catch (Glib::SpawnError& e) {
    auto msg = Glib::ustring::compose(_("Could not execute ffmpeg: %1"),
                                      e.what());
    Gtk::MessageDialog dlg(*this, msg, false, Gtk::MESSAGE_ERROR);
    dlg.run();
    return;
  }

  Glib::signal_child_watch().connect(sigc::mem_fun(*this, &EncodeWindow::ffmpeg_finished),
                                     ffmpeg_pid);
}


void EncodeWindow::ffmpeg_finished(Glib::Pid pid, int status)
{
  Glib::spawn_close_pid(pid);
  ::close(tmp_fd_);
  ::unlink(tmp_filter_file_.c_str());

  Gtk::MessageDialog dlg(*this, _("Encoding finished"));
  dlg.run();

}
