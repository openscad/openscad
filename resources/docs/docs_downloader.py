import os
import shutil
import socket
import urllib.request

url = 'https://files.openscad.org/documentation/'
url_zip = 'https://files.openscad.org/documentation/manual.zip'
url_pdf = ['https://files.openscad.org/documentation/manual/OpenSCAD_User_Manual.pdf', 'https://files.openscad.org/documentation/manual/The_OpenSCAD_Language.pdf']

docs_dir = os.path.join(os.getcwd(), 'openscad_docs')
pdfs_dir = os.path.join(os.getcwd(), 'openscad_docs_pdf')

dns_server = "one.one.one.one"


def is_connected(hostname) -> bool:
    """
    This function checks if an active internet connection
    is available and returns a Boolean value.
    """
    try:
        host = socket.gethostbyname(hostname)
        s = socket.create_connection((host, 80), 10)
        s.close()
        return True
    except:
        pass
    return False


# The following downloads the documentation from
# https://files.openscad.org/documentation/
# if is_connected() returns True
if is_connected(dns_server):
    if not os.path.exists(pdfs_dir):
        os.makedirs(pdfs_dir)
    print(f'Downloading : {url_zip}')
    file_name = url_zip.split('/')[-1]
    urllib.request.urlretrieve(url_zip, file_name)
    shutil.unpack_archive(file_name, docs_dir, 'zip')
    os.remove(file_name)

    for pdf in url_pdf:
        print(f'Downloading : {pdf}')
        file_name = pdf.split('/')[-1]
        file_name = os.path.join(pdfs_dir, file_name)
        urllib.request.urlretrieve(pdf, file_name)
else:
    print('Not connected to the Internet, Skipping download of User Manual.')
