from __future__ import print_function
from __future__ import absolute_import
from __future__ import division
from __future__ import unicode_literals
from fabric.api import local, run, sudo, cd, env, hosts, shell_env
from fabric.contrib.files import exists
import git
from six.moves import shlex_quote

env.hosts = ["venti"]
env.use_ssh_config = True

def cmd(*args):
    return " ".join(shlex_quote(a) for a in args)

@hosts("venti")
def test_venti():
    fedora_cxxflags = "-O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector-strong"\
                      " --param=ssp-buffer-size=4 -grecord-gcc-switches  -m64 -mtune=generic"
    fedora_ldflags = "-Wl,-z,relro"

    repo = git.Repo()
    local(cmd("git", "push", "venti", "HEAD"))
    with cd("~/dballe"):
        #run(cmd("git", "fetch"))
        run(cmd("git", "checkout", "-B", "test_venti", repo.head.commit.hexsha))
        run(cmd("git", "reset", "--hard"))
        run(cmd("git", "clean", "-fx"))
        run(cmd("autoreconf", "-if"))
        run(cmd("./configure",
                "--build=x86_64-redhat-linux-gnu",
                "--host=x86_64-redhat-linux-gnu",
                "--program-prefix=",
                "--prefix=/usr",
                "--exec-prefix=/usr",
                "--bindir=/usr/bin",
                "--sbindir=/usr/sbin",
                "--sysconfdir=/etc",
                "--datadir=/usr/share",
                "--includedir=/usr/include",
                "--libdir=/usr/lib64",
                "--libexecdir=/usr/libexec",
                "--localstatedir=/var",
                "--sharedstatedir=/var/lib",
                "--mandir=/usr/share/man",
                "--infodir=/usr/share/info",
                "CFLAGS=" + fedora_cxxflags,
                "CXXFLAGS=" + fedora_cxxflags,
                "LDFLAGS=" + fedora_ldflags))
        run(cmd("make"))
        run(cmd("./run-check", "TEST_BLACKLIST=*odbc*"))

def test():
    test_venti()
