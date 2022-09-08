import matplotlib.pyplot as plt
import glob

# Process obstacle pngs into binary
out = open('processed_obstacles.dat', 'w')
paths = glob.glob('obstacles/*.png')
paths.insert(0, "start.png")
out.write(str(len(paths)) + '\n')
for im_path in paths:
    im = plt.imread(im_path)
    if im.shape != (30, 32, 4):
        raise Exception(f"Obstacle does not have correct dimensions (32x30): {im_path}")
    
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



# Process player png into binary
out = open('processed_player.dat', 'w')
path = 'player.png'
im = plt.imread(path)
if im.shape != (8, 8, 4):
    raise Exception(f"Player sprite does not have correct dimensions (8x8): {path}")
# Convert to RGBA 0-255 and keep track of number of distinct colors
def to_rgba(c):
    col = []
    for k in range(4):
        col.append(min(255, int(im[i][j][k] * 256)))
    return tuple(col)

colors = set()
for i in range(8):
    for j in range(8):
        colors.add(to_rgba(im[i][j]))
colors = list(colors)

if len(colors) > 4:
    raise Exception(f"Player sprite uses more than 4 colors")

while len(colors) < 4:
    colors.append((0, 0, 0, 0))

# Output the 4 colors we will use
for i in range(4):
    for j in range(4):
        if j != 0:
            out.write(' ')
        out.write(str(colors[i][j]))
    out.write('\n')

# Output the image itself
color_map = dict()
tmp = 0
for c in colors:
    color_map[c] = tmp
    tmp += 1

for i in range(8):
    for j in range(8):
        if j != 0:
            out.write(' ')
        out.write(str(color_map[to_rgba(im[i][j])]))
    out.write('\n')
