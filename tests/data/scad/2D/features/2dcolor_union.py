from openscad import *

result= []
for i in range(12):
    red = (Sin(i*30)+1)/2
    green = (Sin((i+4)*30)+1)/2
    blue = (Sin((i+8)*30)+1)/2
    print(red)
    result.append(circle(10).color([red,green,blue]).right(15).rotz(30*i))

show(result)
