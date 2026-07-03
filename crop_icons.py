from PIL import Image
import os

img = Image.open('/Users/yalazi/Documents/PROJECTS/electronics/s3-inky-expression-4.0/__FS__/weather-icons.png')
w, h = 256, 256
cols, rows = 4, 3

os.makedirs('main/icons', exist_ok=True)
for row in range(rows):
    for col in range(cols):
        idx = row * cols + col
        box = (col * w, row * h, (col + 1) * w, (row + 1) * h)
        cropped = img.crop(box)
        # Resize to 64x64 so it fits easily on the screen and uses very little RAM
        cropped = cropped.resize((64, 64), Image.LANCZOS)
        cropped.save(f'main/icons/icon_{idx}.png')

print("Icons cropped!")
