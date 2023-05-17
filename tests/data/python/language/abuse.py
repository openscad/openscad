import sys, os
# Checking the python-openscad for the worst things a user could potentionally do

try:
    mycube = cube("eat that")
except Exception as ex:
    exc_type, exc_obj, exc_tb = sys.exc_info()
    print(ex, exc_tb.tb_lineno)    
    
mycube=cube()
output(mycube)    

