import matplotlib.pyplot as plt
import numpy as np

fig = plt.figure()
ax = fig.add_subplot(projection='3d')

# Make data
#define corners

corners = [
    (-1, -1,  0),
    (-1,  1,  0),
    ( 1,  1,  0),
    ( 1, -1,  0)
]

x = [c[0] for c in corners]
y = [c[1] for c in corners]
z = [c[2] for c in corners]

# Plot the surface
print(x)
ax.scatter(x,y,z)
 
# Set an equal aspect ratio
ax.set_aspect('equal')

plt.show()