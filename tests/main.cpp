/*
 * Copyright 2011 StreamNovation Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY StreamNovation Ltd. ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL StreamNovation Ltd. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of StreamNovation Ltd.
 *
 *
 * Author(s):
 *          Adam Rak <adam.rak@streamnovation.com>
 *
 *
 *
 */

#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <r800_state.h>

using namespace std;

void do_test(r800_state&);

int open_drm(std::string fname)
{
  int fd = open(fname.c_str(), O_RDWR, 0);

  if (fd < 0)
  {
    throw runtime_error("Error, cannot open DRM device: " + fname);
  }

  return fd;
}

int main(int argc, char* argv[])
{
  const char *card = "/dev/dri/card0";
  bool reset = false;

  for(int i = 1; i < argc; i++) {
    switch((int)argv[i][0]) {
      case '-':
        if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
          cout << "Usage: " << argv[0] << " [-r] [-cxxxx]" << endl;
          cout << "OPTIONS:" << endl;
          cout << "  -r --reset  reset GPU before starting (needs X to be shut down)" << endl;
          cout << "  -c/dev/dri/card1  use alternative card" << endl;
          return 0;
        }
        if(!strcmp(argv[i], "-r") || !strcmp(argv[i], "--reset")) {
          reset = true;
          cout << "Resetting card" << endl;
          break;
        }
        if(argv[i][1] == 'c') {
          card = &argv[i][2];
          cout << "Use alternative card " << card << endl;
          break;
        }
      default:
        cout << "Unrecognized option: " << argv[i] << endl;
        return 1;
    }
  }
  cerr << "Running test: " << argv[0] << endl;

  assert(drmAvailable());

  int drm_fd;

  drm_fd = open_drm(card);
  r800_state state(drm_fd, reset);
  state.set_default_state();

  do_test(state);

}
