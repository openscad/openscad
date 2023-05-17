# Checking the python-openscad for the worst things a user could potentionally do

try:
    mycube = cube("eat that")
except Exception as ex:
    print(ex)      
    
mycube=cube()
output(mycube)    

