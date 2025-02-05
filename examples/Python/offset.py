from openscad import *
c = cube(10) + [[0,0,0], [5,5,5]] # multiple translation

# Fillets can easily be created by downsizing concave edges
c = c.offset(-2,fa=5)
c.show()
