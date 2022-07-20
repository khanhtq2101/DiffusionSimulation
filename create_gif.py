import os
import imageio

if not os.path.isdir('./results/images/'):
  os.system('mkdir ./results/images/')

images = []
for img_name in os.listdir('results/images/'):
    images.append(imageio.imread(os.path.join('results/images/', img_name)))
imageio.mimsave('results/simulation_gif.gif', images)

print("OK")