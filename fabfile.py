from __future__ import print_function
from __future__ import absolute_import
from __future__ import division
from __future__ import unicode_literals
from fabric.api import local, run, sudo, cd, env, hosts, shell_env
from fabric.contrib.files import exists
import git
import re
from six.moves import shlex_quote

env.hosts = ["venti", "ventiquattro", "ventotto", "sette"]
env.use_ssh_config = True


def cmd(*args):
    return " ".join(shlex_quote(a) for a in args)


@hosts("venti")
def test_venti():
    fedora_cxxflags = "-O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector-strong"\
                      " --param=ssp-buffer-size=4 -grecord-gcc-switches  -m64 -mtune=generic"
    fedora_ldflags = "-Wl,-z,relro"

    repo = git.Repo()
    remote = repo.remote("venti")
    push_url = remote.config_reader.get("url")
    remote_dir = re.sub(r"^ssh://[^/]+", "", push_url)

    local(cmd("git", "push", "venti", "HEAD"))
    with cd(remote_dir):
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
        run(cmd("./run-check"))


@hosts("ventiquattro")
def test_ventiquattro():
    fedora_cxxflags = "-O2 -g -pipe -Wall -Werror=format-security -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector-strong"\
                      " --param=ssp-buffer-size=4 -grecord-gcc-switches -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -m64 -mtune=generic"
    fedora_ldflags = "-Wl,-z,relro -specs=/usr/lib/rpm/redhat/redhat-hardened-ld"

    repo = git.Repo()
    remote = repo.remote("ventiquattro")
    push_url = remote.config_reader.get("url")
    remote_dir = re.sub(r"^ssh://[^/]+", "", push_url)

    local(cmd("git", "push", "ventiquattro", "HEAD"))
    with cd(remote_dir):
        run(cmd("git", "checkout", "-B", "test_ventiquattro", repo.head.commit.hexsha))
        run(cmd("git", "reset", "--hard"))
        run(cmd("git", "clean", "-fx"))
        run(cmd("autoreconf", "-if"))
        run(cmd("./configure",
                "--build=x86_64-redhat-linux-gnu",
                "--host=x86_64-redhat-linux-gnu",
                "--program-prefix=",
                "--disable-dependency-tracking",
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
        run(cmd("./run-check"))


@hosts("ventotto")
def test_ventotto():
    fedora_cxxflags = "-O2 -g -pipe -Wall -Werror=format-security -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector-strong"\
                      " --param=ssp-buffer-size=4 -grecord-gcc-switches -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -m64 -mtune=generic"
    fedora_ldflags = "-Wl,-z,relro -specs=/usr/lib/rpm/redhat/redhat-hardened-ld"

    repo = git.Repo()
    remote = repo.remote("ventotto")
    push_url = remote.config_reader.get("url")
    remote_dir = re.sub(r"^ssh://[^/]+", "", push_url)

    local(cmd("git", "push", "ventotto", "HEAD"))
    with cd(remote_dir):
        run(cmd("git", "checkout", "-B", "test_ventotto", repo.head.commit.hexsha))
        run(cmd("git", "reset", "--hard"))
        run(cmd("git", "clean", "-fx"))
        run(cmd("autoreconf", "-if"))
        run(cmd("./configure",
                "--build=x86_64-redhat-linux-gnu",
                "--host=x86_64-redhat-linux-gnu",
                "--program-prefix=",
                "--disable-dependency-tracking",
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
        run(cmd("./run-check"))


@hosts("sette")
def test_sette():
    fedora_cxxflags = "-O2 -g -pipe -Wall -Werror=format-security -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector-strong"\
                      " --param=ssp-buffer-size=4 -grecord-gcc-switches -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -m64 -mtune=generic"
    fedora_ldflags = "-Wl,-z,relro -specs=/usr/lib/rpm/redhat/redhat-hardened-ld"

    repo = git.Repo()
    remote = repo.remote("sette")
    push_url = remote.config_reader.get("url")
    remote_dir = re.sub(r"^ssh://[^/]+", "", push_url)

    local(cmd("git", "push", "sette", "HEAD"))
    with cd(remote_dir):
        run(cmd("git", "checkout", "-B", "test_sette", repo.head.commit.hexsha))
        run(cmd("git", "reset", "--hard"))
        run(cmd("git", "clean", "-fx"))
        run(cmd("autoreconf", "-if"))
        run(cmd("./configure",
                "--build=x86_64-redhat-linux-gnu",
                "--host=x86_64-redhat-linux-gnu",
                "--program-prefix=",
                "--disable-dependency-tracking",
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
        run(cmd("./run-check"))


def test():
    test_venti()
    test_ventiquattro()
