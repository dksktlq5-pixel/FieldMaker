from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
src=Path(r'D:\UnReal\FieldMaker\SourceArt\Blender\CustomWeapons\Icons')
items=['Glock18','USPS','P250','FiveSeven','DesertEagle','MAC10','UMP45','MP5SD','MP7','P90','Nova','Negev','HEGrenade','SmokeGrenade','IncendiaryGrenade','Flashbang','DecoyGrenade']
W,H,C,R=250,170,5,4
out=Image.new('RGB',(W*C,H*R),(30,36,43));d=ImageDraw.Draw(out)
font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',16)
for i,name in enumerate(items):
 im=Image.open(src/f'T_UIWeapon_{name}.png').convert('RGBA')
 alpha=im.getchannel('A'); box=alpha.getbbox()
 if box: im=im.crop(box)
 im.thumbnail((W-24,H-45),Image.Resampling.LANCZOS)
 x=(i%C)*W+(W-im.width)//2;y=(i//C)*H+5+(H-45-im.height)//2
 out.paste(im,(x,y),im)
 label=name.replace('Grenade',' Grenade')
 tw=d.textbbox((0,0),label,font=font)[2]
 d.text(((i%C)*W+(W-tw)//2,(i//C)*H+H-32),label,font=font,fill=(229,194,129))
out.save(src/'WeaponSet_17_Preview.png')
print(src/'WeaponSet_17_Preview.png')
