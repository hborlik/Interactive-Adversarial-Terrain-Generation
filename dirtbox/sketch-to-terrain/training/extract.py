from multiprocessing import Pool
from pathlib import Path
import numpy as np
import random
from PIL import Image
import cv2

import download_data

def chunk(data, chunk_size):
    for i in range(0, len(data), chunk_size):
        yield data[i:i+chunk_size]

def gen_output_filenames(name, num):
    for i in range(num):
        yield f'{name}_{i}.npz'

if __name__ == "__main__":
    regen_sketches = False
    extract_inputs = True
    collect_inputs = False
    test_input_size = 512
    # outputfile = 'input/training_data.npz'
    num_datatiles = 1700
    print("STARTING Tile extraction")
    tiles = []
    for tf in Path('./data/tiles').glob('*.tif'):
        dtf = Path(f'./data/tiles/downsampled/{tf.name}')
        if not dtf.is_file():
            print(f'ERROR: {dtf.absolute()} is not a file')
            exit()
        tiles.append((dtf, tf))
    random.shuffle(tiles)
    print(f'Found {len(tiles)} tiles')
    tiles = tiles[0:num_datatiles]
    payloads = list(chunk(tiles, 10))
    output_filenames = list(gen_output_filenames('data/input/dump', len(payloads)))
    payloads = zip(payloads, output_filenames)
    # download_data.extract_sketches(next(payloads))
    # exit(-1)
    if regen_sketches:
        with Pool(10) as p:
            p.map(download_data.extract_sketches, payloads)
    else:
        print('skipping regen_sketches')

    if extract_inputs:
        i = 0
        for p in Path('./data/input').glob('*.npz'):
            sketch_data = np.load(p)
            for sketch, elevation in zip(sketch_data['x'], sketch_data['y']):
                sketch = sketch[0:0+test_input_size,0:0+test_input_size]
                elevation = elevation[0:0+test_input_size,0:0+test_input_size]
                elevation = np.squeeze(elevation, axis=2)
                elevation = download_data.hillshade(-elevation * 127.5 + 127.5)
                elevation = np.tile(np.reshape(elevation, np.shape(elevation)[0:2])[:, :, None], [1, 1, 3]).astype('uint8')
                im = Image.fromarray(sketch)
                im.save(f'./data/input/test/sketch_{i}.png')
                im = Image.fromarray(elevation)
                im.save(f'./data/input/test/sketch_elevation{i}.png')
                i += 1
        print(i)

    if collect_inputs:
        print('starting collection')
        height_maps = []
        sketch_maps = []
        for cf in output_filenames:
            chunk = np.load(cf)
            height_maps.append(chunk['y'])
            sketch_maps.append(chunk['x'])
        training_output = np.concatenate(height_maps)
        training_input = np.concatenate(sketch_maps)

        print(training_input.shape)
        print(training_output.shape)
        print('done.')

        np.savez(outputfile, x=training_input, y=training_output)