/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

#include <string>
#include "modplugxmms/modplugxmms.h"

void ShowAboutWindow();
void ShowConfigureWindow(const ModplugXMMS::Settings& aProps);
void ShowInfoWindow(const string& aFileName);
