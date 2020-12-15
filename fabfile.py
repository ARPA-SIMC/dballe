from __future__ import print_function
from __future__ import absolute_import
from __future__ import division
from __future__ import unicode_literals
from fabric.api import local, run, cd, env, hosts, put
from io import BytesIO
import git
import re
from six.moves import shlex_quote

env.hosts = ["venti", "ventiquattro", "ventotto", "sette", "otto"]
env.use_ssh_config = True


def cmd(*args):
    return " ".join(shlex_quote(a) for a in args)


def configure():
    res = run(cmd("rpm", "--eval", "%configure --enable-arpae-tests"))
    put(BytesIO(b"\n".join(res.stdout.splitlines())), "rpm-config")
    run(cmd("chmod", "0755", "rpm-config"))
    run(cmd("./rpm-config"))


def push(host):
    repo = git.Repo()
    remote = repo.remote(host)
    push_url = remote.config_reader.get("url")
    remote_dir = re.sub(r"^ssh://[^/]+", "", push_url)

    local(cmd("git", "push", host, "HEAD", "--force"))
    with cd(remote_dir):
        run(cmd("git", "reset", "--hard"))
        run(cmd("git", "clean", "-fx"))
        run(cmd("git", "checkout", "-B", "test_" + host, repo.head.commit.hexsha))
        run(cmd("git", "clean", "-fx"))
        run(cmd("autoreconf", "-if"))
        configure()


def run_test(host):
    push(host)

    repo = git.Repo()
    remote = repo.remote(host)
    push_url = remote.config_reader.get("url")
    remote_dir = re.sub(r"^ssh://[^/]+", "", push_url)
    with cd(remote_dir):
        run(cmd("make", "-j2"))
        run(cmd("make", "check", "-j2", "TEST_VERBOSE=1"))


@hosts("venti")
def test_venti():
    run_test("venti")


@hosts("ventiquattro")
def test_ventiquattro():
    run_test("ventiquattro")


@hosts("ventotto")
def test_ventotto():
    run_test("ventotto")


@hosts("sette")
def test_sette():
    run_test("sette")


@hosts("otto")
def test_otto():
    run_test("otto")


def test():
    test_venti()
    test_ventiquattro()
