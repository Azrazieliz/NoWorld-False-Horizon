"""Reference overlay for Koikatsu: always-on-top comparison window, no GUI automation.
Usage: python koikatsu_reference_overlay.py --image C:\\path\\ref.jpg
"""
import argparse,pathlib,tkinter as tk,sys
from PIL import Image,ImageTk
def start(path):
 root=tk.Tk();root.title('NoWorld | 2D REFERENCE | always on top')
 root.attributes('-topmost',True);root.configure(bg='#15151b')
 root.update_idletasks();screen_x=max(0,root.winfo_screenwidth()-400)
 root.geometry('380x680+%d+75'%screen_x)
 bar=tk.Frame(root,bg='#1e1e26');bar.pack(fill='x')
 label=tk.Label(bar,text='LIVE REFERENCE   •   DRAG IMAGE TO MOVE',bg='#1e1e26',fg='white',font=('Segoe UI',10,'bold'));label.pack(side='left',padx=8,pady=6)
 body=tk.Label(root,bg='#15151b');body.pack(fill='both',expand=True)
 footer=tk.Label(root,text='Use mouse wheel to zoom  |  Right click to toggle topmost',bg='#1e1e26',fg='#aaaeb8');footer.pack(fill='x',pady=4)
 original=Image.open(path).convert('RGB');scale=[1.]
 def draw(event=None):
  w=max(80,root.winfo_width()-12);h=max(80,root.winfo_height()-68)
  im=original.copy();im.thumbnail((int(w*scale[0]),int(h*scale[0])),Image.Resampling.LANCZOS)
  photo=ImageTk.PhotoImage(im);body.configure(image=photo);body.image=photo
 def zoom(event):
  scale[0]=max(.5,min(3.,scale[0]*(1.15 if event.delta>0 else .87)));draw()
 x=[0];y=[0]
 def press(event):x[0]=event.x_root-root.winfo_x();y[0]=event.y_root-root.winfo_y()
 def drag(event):root.geometry('+%d+%d'%(event.x_root-x[0],event.y_root-y[0]))
 def toggle(event):root.attributes('-topmost',not bool(root.attributes('-topmost')))
 for el in (root,body,label):
  el.bind('<Button-1>',press);el.bind('<B1-Motion>',drag);el.bind('<MouseWheel>',zoom);el.bind('<Button-3>',toggle)
 root.bind('<Configure>',lambda event:draw() if event.widget==root else None)
 root.after(120,draw);root.mainloop()
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--image',required=True);a=p.parse_args()
 f=pathlib.Path(a.image).resolve()
 if not f.is_file():raise FileNotFoundError(f)
 start(f)
