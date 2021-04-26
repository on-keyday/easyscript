#    Copyright (c) 2021 on-keyday
#    Released under the MIT license
#    https://opensource.org/licenses/mit-license.php
import os
import licensetools as lic
from pathlib import Path
import re

def license_to_bat(license,lists):
    return lic.onelinecomment_license("rem ","rem license",license,lists)

def license_to_shell_or_txt(license,lists):
    return lic.onelinecomment_license(license=license,lists=lists)

def license_to_cpp(license,lists):
    return lic.blockcomment_license(license=license,lists=lists)

def CheckPath(pa:Path,ch:Path):
    if ch == pa:
        return False
    else:
        try:
            return CheckPath(pa,ch.parents[0])
        except:
            return True 

def IsSharpComment(filename:str)->bool:
    return filename.endswith(".txt") or filename.endswith(".sh") or filename.endswith(".py")

def IsBlockComment(filename:str)->bool:
    return filename.endswith(".cpp") or filename.endswith(".h") or filename.endswith(".c")

def IsOnlyCopy(filename:str)->bool:
    return filename.endswith(".json") or filename.endswith(".gitignore") or filename.endswith(".gitkeep")

def Nocopy(filename:str)->bool:
    if not filename.endswith(".pyc"):
        if filename.endswith(".gitkeep"):
            return True
        filepath=Path(filename)
        while True:
            try:
                if filepath.parents[0].name==".git":
                    return True
                elif filepath.parents[0].name=="__pycache__":
                    return True    
                elif (filepath.name=="src" or filepath.name=="built") and filepath.parents[0].name.endswith("_build"):
                    return True
                filepath=filepath.parents[0]
            except:
                return False
    return True
            



def copyfiles(copyfrom="",copyto="",licfile=""):
    os.chdir(copyfrom)
    frompath=Path(copyfrom).resolve()
    topath=Path(copyto).resolve()
    if not CheckPath(frompath,topath):
        print(copyto+" is child dir of "+copyfrom)
        exit(-1)
    
    batlist=list()
    sharplist=list()
    blocklist=list()
    for file in frompath.glob("**/*"):
        if not file.is_dir():
            filename=str(file.relative_to(copyfrom))
            if Nocopy(filename):
                continue
            copyfile=topath.joinpath(filename)
            lic.try_copy(filename,copyfile)    
            if IsOnlyCopy(filename):
                continue
            elif filename.endswith(".bat"):
                batlist.append(copyfile)
            elif IsBlockComment(filename):
                blocklist.append(copyfile)
            elif IsSharpComment(filename):
                sharplist.append(copyfile)
            else:
                sharplist.append(copyfile)
    template=lic.gettemplate(licfile)
    license_to_bat(template,batlist)
    license_to_shell_or_txt(template,sharplist)
    license_to_cpp(template,blocklist)

if __name__ == "__main__":
    import sys
    copyfrom=os.getcwd()
    copyto=sys.argv[1]
    licfile=sys.argv[2]
    copyfiles(copyfrom,copyto,licfile)