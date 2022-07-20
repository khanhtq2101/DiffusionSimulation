import numpy as np
import matplotlib.pyplot as plt
import cv2
import os

if not os.path.isdir('./results/images/'):
  os.system('mkdir ./results/images/')

for name in os.listdir('./results/CUDA/'):
    file_path = os.path.join('./results/CUDA/', name)
    f = open(file_path)
    heat_cuda = np.zeros((256, 256), np.float64)

    for i in range(256):
            line = f.readline()
            if not line:
                    break
            line = line.split('\t')[:-1]	
            for j in range(256):
                    heat_cuda[i, j] = float(line[j])

    plt.imsave(os.path.join('./results/images/', name[:-4] + '.png'), heat_cuda, vmin = 0, vmax = 100)

print("OK")
