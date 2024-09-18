
import matplotlib.pyplot as plt

# Create a figure and axis
fig, ax = plt.subplots()

# Draw two circles (representing abstract objects)
circle1 = plt.Circle((0.4, 0.5), 0.1, color='blue', fill=True)
circle2 = plt.Circle((0.6, 0.5), 0.1, color='blue', fill=True)

# Draw a rectangle (representing a shaft-like form)
rect = plt.Rectangle((0.45, 0.12), 0.1, 0.4, color='blue')

# Add the shapes to the plot
ax.add_patch(circle1)
ax.add_patch(circle2)
ax.add_patch(rect)

# Set the aspect ratio of the plot to be equal
ax.set_aspect('equal')

# Set limits and remove axes for aesthetics
plt.xlim(0, 1)
plt.ylim(0, 1)
plt.axis('off')

# Show the plot
plt.show()
