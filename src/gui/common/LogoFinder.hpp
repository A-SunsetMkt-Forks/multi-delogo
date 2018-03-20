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
#ifndef MDL_LOGO_FINDER_H
#define MDL_LOGO_FINDER_H


namespace mdl {
  class LogoFinderResult
  {
  public:
    int start_frame;
    int x;
    int y;
    int width;
    int height;
  };


  class LogoFinderCallback
  {
  public:
    virtual bool success(const LogoFinderResult& result) = 0;
    virtual bool failure(int start_frame) = 0;
  };


  class LogoFinder
  {
  public:
    LogoFinder(LogoFinderCallback& callback)
      : callback_(callback) { };

    virtual ~LogoFinder() { };


    void set_start_frame(int start_frame) {
      start_frame_ = start_frame;
    }

    void set_frame_interval_min(int frame_interval_min) {
      frame_interval_min_ = frame_interval_min;
    }

    void set_extra_frames(int extra_frames) {
      extra_frames_ = extra_frames;
    }


    int get_min_logo_width() const {
      return min_logo_width_;
    }

    void set_min_logo_width(int min_logo_width) {
      min_logo_width_ = min_logo_width;
    }

    int get_max_logo_width() const {
      return max_logo_width_;
    }

    void set_max_logo_width(int max_logo_width) {
      max_logo_width_ = max_logo_width;
    }

    int get_min_logo_height() const {
      return min_logo_height_;
    }

    void set_min_logo_height(int min_logo_height) {
      min_logo_height_ = min_logo_height;
    }

    int get_max_logo_height() const {
      return max_logo_height_;
    }

    void set_max_logo_height(int max_logo_height) {
      max_logo_height_ = max_logo_height;
    }


    virtual void find_logos() = 0;

  protected:
    int start_frame_;
    int frame_interval_min_;
    int extra_frames_;

    /**
     * Minimal box width to be recognized as a possible logo.
     */
    int min_logo_width_ = 61;
    /**
     * Maximal box width to be recognized as a possible logo.
     */
    int max_logo_width_ = 159;
    /**
     * Minimal box height to be recognized as a possible logo.
     */
    int min_logo_height_ = 9;
    /**
     * Maximal box height to be recognized as a possible logo.
     */
    int max_logo_height_ = 23;


    LogoFinderCallback& callback_;
  };
}


#endif // MDL_LOGO_FINDER_H
