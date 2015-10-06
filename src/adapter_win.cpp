/**
 *
 *
 *
 * Portions of this code is from the node-serialport project: https://github.com/voodootikigod/node-serialport
 *
 * The license that code is release under is:
 *
 * Copyright 2010, 2011, 2012 Christopher Williams. All rights reserved.
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include "adapter.h"

#include <list>

#include "win/disphelper.h"
#include "win/stdafx.h"
#include "win/enumser.h"

#include <nan.h>

#define MAX_BUFFER_SIZE 1000

/*
 * listComPorts.c -- list COM ports
 *
 * http://github.com/todbot/usbSearch/
 *
 * 2012, Tod E. Kurt, http://todbot.com/blog/
 *
 *
 * Uses DispHealper : http://disphelper.sourceforge.net/
 *
 * Notable VIDs & PIDs combos:
 * VID 0403 - FTDI
 *
 * VID 0403 / PID 6001 - Arduino Diecimila
 *
 */
void GetAdapterList(uv_work_t* req) {
    AdapterListBaton* data = static_cast<AdapterListBaton*>(req->data);

    {
        DISPATCH_OBJ(wmiSvc);
        DISPATCH_OBJ(colDevices);

        dhInitialize(TRUE);
        dhToggleExceptions(FALSE);

        dhGetObject(L"winmgmts:{impersonationLevel=impersonate}!\\\\.\\root\\cimv2", NULL, &wmiSvc);
        dhGetValue(L"%o", &colDevices, wmiSvc, L".ExecQuery(%S)", L"Select * from Win32_PnPEntity");

        FOR_EACH(objDevice, colDevices, NULL) {
            char* name = NULL;
            char* pnpid = NULL;
            char* manu = NULL;
            char* match;

            dhGetValue(L"%s", &name,  objDevice, L".Name");
            dhGetValue(L"%s", &pnpid, objDevice, L".PnPDeviceID");

            if( name != NULL && ((match = strstr( name, "(COM" )) != NULL) ) { // look for "(COM23)"
                // 'Manufacturuer' can be null, so only get it if we need it
                dhGetValue(L"%s", &manu, objDevice,  L".Manufacturer");

                if(strcmp("SEGGER", manu) == 0)
                {
                    char* comname = strtok( match, "()");
                    AdapterListResultItem* resultItem = new AdapterListResultItem();
                    resultItem->comName = comname;
                    resultItem->manufacturer = manu;
                    resultItem->pnpId = pnpid;
                    data->results.push_back(resultItem);
                }

                dhFreeString(manu);
              }

            dhFreeString(name);
            dhFreeString(pnpid);
        } NEXT(objDevice);

        SAFE_RELEASE(colDevices);
        SAFE_RELEASE(wmiSvc);

        dhUninitialize(TRUE);
    }

#if 0
  std::vector<UINT> ports;
  if (CEnumerateSerial::UsingQueryDosDevice(ports))
  {
    for (size_t i = 0; i < ports.size(); i++)
    {
      char comname[64] = { 0 };
      _snprintf(comname, sizeof(comname), "COM%u", ports[i]);
      bool bFound = false;
      for (std::list<AdapterListResultItem*>::iterator ri = data->results.begin(); ri != data->results.end(); ++ri)
      {
        if (stricmp((*ri)->comName.c_str(), comname) == 0)
        {
          bFound = true;
          break;
        }
      }
      if (!bFound)
      {
        AdapterListResultItem* resultItem = new AdapterListResultItem();
        resultItem->comName = comname;
        resultItem->manufacturer = "";
        resultItem->pnpId = "";
        data->results.push_back(resultItem);
      }
    }
  }
#endif
}
