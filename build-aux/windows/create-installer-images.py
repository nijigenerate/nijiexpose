from pathlib import Path
from PIL import Image, ImageFilter

root = Path(__file__).resolve().parents[2]
assets = root / 'build-aux/windows/wix/res'
assets.mkdir(parents=True, exist_ok=True)
# Original artwork: Aka-20240109-1.3-flat.psd (embedded merged image).
# Cropped original pixels; no generated artwork or repainting.
head = Image.open(assets / 'aka-2024-head.png').convert('RGBA')
logo = Image.open(root / 'res/logo.png').convert('RGBA')
icon = Image.open(root / 'res/icon_x256.png').convert('RGBA')

def scaled(image, width):
    return image.resize((width, round(image.height * width / image.width)), Image.Resampling.LANCZOS)

def save(image, name):
    image.convert('RGB').save(assets / (name + '.bmp'))
    image.convert('RGB').save(assets / (name + '.png'))

# WiX welcome/finish dialogs reserve the right two thirds for controls.
dialog = Image.new('RGBA', (493, 312), (240, 240, 240, 255))
dialog.alpha_composite(scaled(logo, 150), (5, 3))
# Use the existing application icon, including its cropped head and emblem.
dialog.alpha_composite(scaled(icon, 144), (10, 168))
save(dialog, 'dialog')

# Keep the header title area white; character and emblem sit at the right edge.
banner = Image.new('RGBA', (493, 58), 'white')
# Match the large face crop in nijigenerate's banner, with both eyes in view.
portrait = scaled(head, 224)
banner.alpha_composite(portrait, (290, -129))
# Blur the full character image, not just its alpha, matching nijigenerate's
# header treatment. Composite the crisp application icon afterwards.
banner = banner.filter(ImageFilter.GaussianBlur(5))
banner.alpha_composite(scaled(icon, 58), (435, 0))
save(banner, 'banner')
