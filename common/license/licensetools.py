#    Copyright (c) 2021 on-keyday
#    Released under the MIT license
#    https://opensource.org/licenses/mit-license.php
import os
import shutil as sh
from pathlib import Path

def rewrite(right="",change="",lists=os.listdir()):
    for filename in lists:
        fdetail=""
        with open(filename,mode="r",encoding="utf-8") as fp:
            fdetail=str(fp.read())
            fdetail=fdetail.replace(change,right,1)
        with open(filename,mode="w",encoding="utf-8") as fp:
            fp.write(fdetail)
    return

def removeifexp(exp="",lists=list()):
    ret=list()
    for file in lists:
        base,exps=os.path.splitext(file)
        if(exps!=exp):
            ret.append(base+exps)
    return ret


def blockcomment_license(begin="/*",end="*/",keyword="/*license*/",license="",lists=list()):
    replace=begin+"\n"+license+"\n"+end+"\n"
    rewrite(replace,keyword,lists)
    return

def onelinecomment_license(begin="#",keyword="#license",license="",lists=list()):
    forvba=begin+license.replace("\n","\n"+begin)
    rewrite(forvba,keyword,lists)
    return

def gettemplate(filename=""):
    right=""
    try:
        with open(filename,mode="r",encoding="utf-8") as fp:
            right=str(fp.read())
    except:
        return False
    return right

def try_makedir(path:Path):
    try:
        os.mkdir(path.as_posix())
    except:
        try_makedir(path.parents[0])
        os.mkdir(path.as_posix())


def try_copy(from_path="",to_path=""):
    try:
        sh.copy2(from_path,to_path)
    except:
        try_makedir(Path(os.path.dirname(to_path)))
        sh.copy2(from_path,to_path)
    return True

if __name__ == "__main__":
    print("not callable")