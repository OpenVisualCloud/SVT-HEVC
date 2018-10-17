# Imports
import sys
import os
import shutil

print "Scrubbing Make"

for path, dirs, j in os.walk('.'):
    for d in dirs:
        for file_name in os.listdir(os.path.join(path, d)):
            #print os.path.join(path, d, file_name)
            if(os.path.splitext(file_name)[1] == '.make'):
                #print os.path.join(path, d, file_name)
                #with open(os.path.join(path, d, file_name), "w+") as f:
                f = open(os.path.join(path, d, file_name), "r")
                a = f.read()
                f.close()

                f = open(os.path.join(path, d, file_name), "w")
                f.write(a.replace('-f elf '," "))
                f.close()
                
