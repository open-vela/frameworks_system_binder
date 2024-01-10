/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <binder/ICpcServiceManager.h>
#include <binder/TextOutput.h>

#include <getopt.h>
#include <libgen.h>

using namespace android;

extern "C" int main(int argc, char* const argv[])
{
    bool wantsUsage = false;
    int result = 0;

    /* Strip path off the program name. */
    char* prog_name = basename(argv[0]);

    while (1) {
        int ic = getopt(argc, argv, "h?");
        if (ic < 0)
            break;

        switch (ic) {
        case 'h':
        case '?':
            wantsUsage = true;
            break;
        default:
            aerr << prog_name << ": Unknown option -" << ic << endl;
            wantsUsage = true;
            result = 10;
            break;
        }
    }

    sp<IServiceManager> sm = defaultCpcServiceManager();

    if (sm == nullptr) {
        aerr << prog_name << ": Unable to get default cpc service manager!" << endl;
        return 20;
    }

    if (optind >= argc) {
        wantsUsage = true;
    } else if (!wantsUsage) {
        if (strcmp(argv[optind], "list") == 0) {
            Vector<String16> services = sm->listServices();
            aout << "Found " << services.size() << " services:" << endl;
            for (unsigned i = 0; i < services.size(); i++) {
                aout << i << "\t" << services[i] << endl;
            }
        } else {
            aerr << prog_name << ": Unknown command " << argv[optind] << endl;
            wantsUsage = true;
            result = 10;
        }
    }

    if (wantsUsage) {
        aout << "Usage: " << prog_name << " [-h|-?]" << endl
             << "       " << prog_name << " list" << endl;
        return result;
    }

    return result;
}
