// $Id: xxMouseWheelViewport.hh 7631 2022-02-12 19:22:07Z flaterco $

/*  xxMouseWheelViewport  Nontrivial callback to make a viewport
    responsive to the mouse wheel (and keyboard).  There might be an
    easier way.

    Copyright (C) 2007  David Flater.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

class xxMouseWheelViewport {
public:

  virtual ~xxMouseWheelViewport();

  // Callback that translates Button4 and Button5 events (or key presses)
  // into actions on the scrollbar.
  void mouseButton (const XButtonEvent *xbe);
  void keyboard (KeySym key);

protected:
  xxMouseWheelViewport();

  // The scrollbar widget is owned by the viewport.  This variable
  // must be initialized by the subclass before the callbacks are
  // installed.
  Widget scrollbarWidget;

  // Install callback for the specified widget.  Widgets needing this
  // are the popup, the scrollbar, and sometimes whatever is inside of
  // the viewport.
  void obeyMouseWheel (Widget widget);

  // Do the scrolling.
  enum HowScroll {UpLine, UpPage, DownLine, DownPage, WayUp, WayDown};
  void scroll (HowScroll how);

private:
  // Prohibited operations not implemented.
  xxMouseWheelViewport (const xxMouseWheelViewport &) = delete;
  xxMouseWheelViewport &operator= (const xxMouseWheelViewport &) = delete;
};