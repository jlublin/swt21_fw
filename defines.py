import subprocess

Import('env')

defines = []
git_mod = subprocess.check_output(
	'[ -z "$$(git status --short)" ] || echo -dirty', shell=True).decode().strip()

git_tag = subprocess.check_output(
	'git describe --tags', shell=True).decode().strip() + git_mod

git_rev = subprocess.check_output(
	'git rev-parse HEAD', shell=True).decode().strip() + git_mod

defines.append(('GIT_TAG', '\\"' + git_tag + '\\"'))
defines.append(('GIT_REV', '\\"' + git_rev + '\\"'))

for define in defines:
	env.Append(CPPDEFINES=('build_flags', '-D{}={}'.format(define[0], define[1])))

