import numpy as np
import matplotlib.pyplot as plt
import cv2
import os

if not os.path.isdir('./results/images/'):
  os.system('mkdir ./results/images/')

path = "./results/images"
img_names = ['00000.png', '00500.png', '01500.png', '11090.png']

fig, axes = plt.subplots(1, 4)
fig.set_size_inches(12, 3)
axes[0].set_title("Original state")
axes[1].set_title("Iteration 500")
axes[2].set_title("Iteration 1500")
axes[3].set_title("Final state")

for i, name in enumerate(img_names):
    img = cv2.imread(os.path.join(path, name))
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    axes[i].imshow(img)
    axes[i].set_axis_off()

plt.savefig('./results/process_figure.png')
plt.show()

print("OK")