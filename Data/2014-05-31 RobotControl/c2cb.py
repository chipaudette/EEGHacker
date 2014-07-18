# -*- coding: utf-8 -*-
"""
Created on Tue Jul 01 14:59:44 2014

@author: mpu
"""
from matplotlib.pyplot import gcf
import win32clipboard
import tempfile
from PIL import Image
import os
from cStringIO import StringIO

def c2cb(fformat='png'):
    fig = gcf()
    fid = tempfile.NamedTemporaryFile(suffix=fformat, delete=False)
    fid.close()
    fig.savefig(fid.name, format=fformat)
    image = Image.open(fid.name)
    output = StringIO()
    image.convert("RGB").save(output, "BMP")
    data = output.getvalue()[14:]
    output.close()

    send_to_clipboard(win32clipboard.CF_DIB, data)
    del image

    os.remove(fid.name)

def send_to_clipboard(clip_type, data):
    win32clipboard.OpenClipboard()
    win32clipboard.EmptyClipboard()
    win32clipboard.SetClipboardData(clip_type, data)
    win32clipboard.CloseClipboard()
