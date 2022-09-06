import matplotlib.pyplot as plt
import glob

out = open('processed_obstacles.dat', 'w')

# png reading from https://stackoverflow.com/questions/31386096/importing-png-files-into-numpy
paths = glob.glob('raw_obstacles/*.png')
out.write(str(len(paths)) + '\n')
for im_path in paths:
    im = plt.imread(im_path)
    if im.shape != (30, 32, 4):
        raise Exception(f"Obstacle does not have correct dimensions: {im_path}")
    
    # convert to 32x30 array of 0s and 1s for easier thinking
    im2 = [ [0] * 30 for i in range(32) ]
    for i in range(32):
        for j in range(30):
            im2[i][j] = 1 - int(im[j][i][0])
    
    # write to output
    for i in range(32):
        for j in range(30):
            if j != 0:
                out.write(' ')
            out.write(str(im2[i][j]))
        out.write('\n')
