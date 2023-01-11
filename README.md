### Application Framework Binder
This is an undergoing work, publication is only intended for developers to review and provide feedback.

### License
As of today, the code is licensed under GLPv3. While no decision as been taken yet, it will certainly switch to a different licence: GPLv2, Apache or MIT e.g..

Final goal is to keep the engine publicly accessible and modifiable, still allowing people to load non open-source plugins. The code itself already leverages open-source libraries, including libmicrohttpd & libjson.

Finally, whatever license is chosen, it should be compatible with dependencies and automotive industry requirements - as the primary target for this code is AGL. 

### Building
Building Application Framework Binder has been tested under **Ubuntu 16.04 LTS (Xenial Xerus)** or **Fedora 23**, and requires the following libraries:
 * libmagic ("libmagic-dev" under Ubuntu, "file-devel" under Fedora);
 * libmicrohttpd >= 0.9.48  (fetch and build from "http://ftp.gnu.org/gnu/libmicrohttpd");
 * json-c ("libjson-c-dev/devel");
 * uuid ("uuid-dev/libuuid-devel");
 * openssl ("libssl-dev/openssl-devel");
 * systemd >= 222 ("libsystemd-dev/systemd-devel");

optionally, for plugins :
 * alsa ("libasound2-dev/alsa-devel");
 * pulseaudio ("libpulse-dev/libpulse-devel");
 * rtl-sdr >= 0.5.0 ("librtlsdr-dev", or fetch and build from "git://git.osmocom.org/rtl-sdr" under Fedora);
 * GUPnP ("libglib2.0-dev libgupnp-av-1.0-dev/glib2-devel libgupnp-av-devel");

Libmicrohttpd should be v49 patched or higher version including patch 5583
 * Prepatched v49 for AGL is avaliable at http://iot.bzh/download/public/2016/afb-demos/libmicrohttpd-0.9.49-agl.tgz
 * Bug is described at https://gerrit.automotivelinux.org/gerrit/#/c/5583/


and the following tools:
 * gcc;
 * make;
 * pkg-config;
 * cmake >= 2.8.8.

To install all dependencies under Ubuntu (excepting libmicrohttpd), please type:
```
$ apt-get install libmagic-dev libjson-c-dev uuid-dev libsystemd-dev libssl-dev libasound2-dev libpulse-dev librtlsdr-dev libglib2.0-dev libgupnp-av-1.0-dev gcc make pkg-config cmake
```
or under Fedora (excepting libmicrohttpd and rtl-sdr):
```
$ dnf install git passwd iproute openssh-server openssh-client openssh-server # Tools needed on top of Docker Minimal Fedora
$ dnf install file-devel gcc gdb make pkgconfig cmake  # install gcc developement tool chain + cmake
$ dnf install file-devel json-c-devel libuuid-devel systemd-devel openssl-devel 
$ dnf install alsa-lib-devel pulseaudio-libs-devel glib2-devel gupnp-av-devel # optional but require to build audio plugin
```

 To build, move to your HOME directory and type:
```
$ export LIBMICRODEST=/opt/libmicrohttpd-0.9.49-agl
$ wget http://iot.bzh/download/public/2016/afb-demos/libmicrohttpd-0.9.49-agl.tgz
$ tar -xzf libmicrohttpd-0.9.49-agl.tgz
$ cd libmicrohttpd-0.9.49-agl
$ ./configure --prefix=$LIBMICRODEST
$ make
$ sudo make install-strip
$ sudo ln -s /opt/libmicrohttpd-0.9.49-agl/lib/libmicrohttpd.so.12.37.0 /usr/lib/x86_64-linux-gnu/libmicrohttpd.so

$ git clone https://github.com/iotbzh/afb-daemon.git
$ cd afb-daemon
$ mkdir build; cd build<br />
$ export PKG_CONFIG_PATH=$LIBMICRODEST/lib/pkgconfig 
$ cmake ..<br />
$ make; 
$ sudo make install<br />
```

### Testing/Debug
```
$ AFB_DAEMON_DIR=$HOME/afb-daemon
$ $AFB_DAEMON_DIR/build/src/afb-daemon --help
$ $AFB_DAEMON_DIR/build/src/afb-daemon --port=1234 --token='' --ldpaths=$AFB_DAEMON_DIR/build --sessiondir=/tmp --rootdir=$AFB_DAEMON_DIR/test 
```

### Starting
```
$ afb-daemon --help 
$ afb-daemon --verbose --port=<port> --token='' --sessiondir=<working directory> --rootdir=<web directory (index.html)>
```

### Example
```
$ afb-daemon --verbose --port=1234 --token='' --sessiondir=/tmp --rootdir=/srv/www/htdocs --alias=icons:/usr/share/icons
```

### Directories & Paths
Default behaviour is to locate ROOTDIR in $HOME/.AFB

### REST API

Developers are intended to provide a structure containing : API name, corresponding methods/callbacks, and optionally a context and a handle.
A handle is a void* structure automatically passed to API callbacks. Callbacks also receive HTTP GET data as well as HTTP POST data, in case a POST method was used. Every method should return a JSON object or NULL in case of error.

API plugins can be protected from timeout and other errors. By default this behaviour is deactivated, use --apitimeout to activate it.
        
        STATIC AFB_restapi myApis[]= {
          {"ping"    , AFB_SESSION_NONE,  (AFB_apiCB)ping,     "Ping Function"},
          {"action1" , AFB_SESSION_CHECK, (AFB_apiCB)action1 , "Action-1"},
          {"action2" , AFB_SESSION_CHECK, (AFB_apiCB)action2 , "Action-2"},
          {NULL}
        };

        PUBLIC AFB_plugin *pluginRegister () {
            AFB_plugin *plugin = malloc (sizeof (AFB_plugin));
            plugin->type  = AFB_PLUGIN_JSON;
            plugin->info  = "Plugin Sample";
            plugin->prefix= "myPlugin";        
            plugin->apis  = myApis;
            return (plugin);
        }

### HTML5 and AngularJS Redirects

Binder supports HTML5 redirect mode even with an application baseurl. Default value for application base URL is /opa.
See Application Framework HTML5 Client template at https://github.com/iotbzh/afb-client-sample

If the Binder receives something like _http://myopa/sample_ when sample is not the homepage of the AngularJS OPA, it will redirect to _http://myopa/#!sample_. This redirect will return the _index.html_ OPA file and will notify AngularJS not to display the homepage, but the sample page.

Warning: in order for AngularJS applications to be able to work with both BASEURL="/" and BASEURL="/MyApp/", all page references have to be relative.
Recommended model is to develop with a BASEURL="/opa" as any application working with a BASEURL will work without, while the opposite is not true.

Note: If a resource is not accessible from ROOTDIR then the "--alias" switch should be used, as in: --alias=/icons:/usr/share/icons. Only use alias for external support static files. This should not be used for API and OPA.


### Ongoing work

Javascript plugins. As of today, only C plugins are supported, but JS plugins are on the TODO list.

