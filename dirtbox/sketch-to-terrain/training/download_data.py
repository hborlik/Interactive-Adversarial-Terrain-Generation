import elevation
import os
import numpy as np
import matplotlib.pyplot as plt
import utm
import cv2
import pathlib
from pathlib import Path
from pysheds.grid import Grid
from skimage.morphology import skeletonize, binary_closing, binary_opening
import scipy
import scipy.ndimage as ndimage
import scipy.ndimage.filters as filters
from rasterio.transform import from_bounds, from_origin
from rasterio.warp import reproject, Resampling, transform
from rasterio.windows import Window
import rasterio

from pyproj import CRS

def download_dem_data(bounds, dem_path):
    output = os.path.join(os.getcwd(), dem_path)
    elevation.clip(bounds=bounds, output=output, product='SRTM1', margin='8%')

def hillshade(array, azimuth = 30, angle_altitude = 30):

    # Source: http://geoexamples.blogspot.com.br/2014/03/shaded-relief-images-using-gdal-python.html

    x, y = np.gradient(array)
    slope = np.pi/2. - np.arctan(np.sqrt(x*x + y*y))
    aspect = np.arctan2(-x, y)
    azimuthrad = azimuth*np.pi / 180.
    altituderad = angle_altitude*np.pi / 180.


    shaded = np.sin(altituderad) * np.sin(slope) \
     + np.cos(altituderad) * np.cos(slope) \
     * np.cos(azimuthrad - aspect)
    return 255*(shaded + 1)/2

def create_preview(dem_path):
    # based on http://geologyandpython.com/dem-processing.html
    with rasterio.open(dem_path) as dem_raster:
        print(dem_raster.bounds)
        west, south, east, north = dem_raster.bounds
        src_crs = dem_raster.crs
        src_shape = src_height, src_width = dem_raster.shape
        src_transform = from_bounds(west, south, east, north, src_width, src_height)
        source = dem_raster.read(1)

        print('LAT/LON:' + str(north) + ' ' + str(west))
        dst_utm = utm.from_latlon(north, west)
        print(dst_utm)
        crs = CRS(proj='utm', zone=dst_utm[2], ellps='WGS84')

        dst_crs_EPSG = ':'.join(crs.to_authority())
        print('Using: ' + dst_crs_EPSG)
        dst_crs = {'init': dst_crs_EPSG}
        # dst_crs = {'init': 'EPSG:4326'}

        # x is lat y is lon
        ctrl_pts = transform(src_crs, dst_crs, [west, east], [north, south])
        print(ctrl_pts)
        
        extent = xmin, xmax, ymin, ymax = ctrl_pts[0][0], ctrl_pts[0][1], ctrl_pts[1][1], ctrl_pts[1][0]
        
        x_extent = xmax - xmin
        y_extent = ymax - ymin

        xsize = 250
        ysize = 250

        print((y_extent / ysize, x_extent / xsize))
        output_shape = (int(y_extent / ysize), int(x_extent / xsize))

        dst_transform = from_origin(xmin, ymax, xsize, ysize)
        dem_array = np.zeros(output_shape, dtype=np.float32)


        reproject(source,
            dem_array,
            src_transform=src_transform,
            src_crs=src_crs,
            dst_transform=dst_transform,
            dst_crs=dst_crs,
            resampling=Resampling.bilinear)

        vmin = np.min(source)
        vmax = np.max(source)

        topocmap = 'Spectral_r'
        fig = plt.figure(figsize=(12, 8))
        ax = fig.add_subplot(111)
        ax.matshow(hillshade(dem_array, 30, 30), extent=extent, cmap='Greys', alpha=.5, zorder=10)
        cax = ax.contourf(dem_array, np.arange(vmin, vmax, 10),extent=extent, 
                        cmap=topocmap, vmin=vmin, vmax=vmax, origin='image')
        fig.colorbar(cax, ax=ax)
        fig.savefig('2.png', dpi=100)

def create_utm_preview(name, dem_path, bounds=None):
    print(f'UTM PREVIEW FILE {name} at {dem_path}')
    with rasterio.open(dem_path) as dem_raster:
        source = dem_raster.read(1)

        west, south, east, north = dem_raster.bounds if bounds is None else bounds
        extent = xmin, xmax, ymin, ymax = west, east, south, north

        vmin = np.min(source)
        vmax = np.max(source)

        topocmap = 'Spectral_r'
        fig = plt.figure(figsize=(12, 8))
        ax = fig.add_subplot(111)
        ax.matshow(hillshade(source, 30, 30), extent=extent, cmap='Greys', alpha=.5, zorder=10)
        cax = ax.contourf(source, np.arange(vmin, vmax, 10),extent=extent, 
                        cmap=topocmap, vmin=vmin, vmax=vmax, origin='image')
        fig.colorbar(cax, ax=ax)
        fig.savefig(f'data/{name}_preview.png', dpi=500)


def to_utm(dem_raster, bounds):
    print(bounds)
    west, south, east, north = bounds
    src_crs = dem_raster.crs
    src_shape = src_height, src_width = dem_raster.shape
    # src_transform = from_bounds(west, south, east, north, src_width, src_height)
    src_transform = dem_raster.transform
    source = dem_raster.read(1)

    print('Finding UTM at LAT/LON:' + str(north) + ' ' + str(west))
    dst_utm = utm.from_latlon(north, west)
    print(dst_utm)
    crs = CRS(proj='utm', zone=dst_utm[2], ellps='WGS84')

    dst_crs_EPSG = ':'.join(crs.to_authority())
    print('Got UTM: ' + dst_crs_EPSG)
    dst_crs = {'init': dst_crs_EPSG}
    # dst_crs = {'init': 'EPSG:4326'}

    # x is lat y is lon
    ctrl_pts = transform(src_crs, dst_crs, [west, east], [north, south])
    # print(f'UTM Bounding: {ctrl_pts}')
    
    extent = xmin, xmax, ymin, ymax = ctrl_pts[0][0], ctrl_pts[0][1], ctrl_pts[1][1], ctrl_pts[1][0]
    
    x_extent = xmax - xmin
    y_extent = ymax - ymin

    xsize = 25
    ysize = 25

    print(f'UTM Destination size {(y_extent / ysize, x_extent / xsize)}')
    output_shape = (int(y_extent / ysize), int(x_extent / xsize))

    dst_transform = from_origin(xmin, ymax, xsize, ysize)
    dem_array = np.zeros(output_shape, dtype=np.float32)


    reproject(source,
        dem_array,
        src_transform=src_transform,
        src_crs=src_crs,
        dst_transform=dst_transform,
        dst_crs=dst_crs,
        resampling=Resampling.bilinear)

    vmin = np.min(source)
    vmax = np.max(source)

    return dem_array, dst_transform, dst_crs, (xmin, xmax, ymin, ymax)

# patch_size and slide in pixles
# returns patches and meters per pixel (x, y)
def extract_patches_from_raster(name, utm_data_file, tile_dir, patch_size, slide):
    count = 0
    discard = 0
    print(f'Creating patches for {utm_data_file}')
    patches = []
    trs = []
    with rasterio.open(utm_data_file) as src:
        width = src.bounds[2] - src.bounds[0]
        height = src.bounds[3] - src.bounds[1]
        crs = src.crs
        print(f'Input size in meters {(width, height)}')
        mpp = (width/src.width, height/src.height)
        print(f'Input size in pixels {(src.width, src.height)}')
        print(f'Patch size in meters is {(mpp[0] * patch_size[0], mpp[1] * patch_size[1])}')
        max_x = src.width - patch_size[0]
        max_y = src.height - patch_size[1]
        for x in range(0, max_x, slide):
            for y in range(0, max_y, slide):
                window = Window(x, y, *patch_size)
                w = src.read(1, window=window)
                if np.mean(w) < 5:
                    discard = discard + 1
                    continue
                patches.append(w)
                trs.append(src.window_transform(window))

    # dump raster data to disk
    for p, t in zip(patches, trs):
        filename = f'{tile_dir}/{name}_tile{count}.tif'
        filename_small = f'{tile_dir}/downsampled/{name}_tile{count}.tif'
        with rasterio.open(
            filename,
            mode='w',
            height=p.shape[0],
            width=p.shape[1],
            count=1,
            dtype=p.dtype,
            driver='GTiff',
            crs=crs,
            transform=t
        ) as dataset_utm:
            dataset_utm.write(p, 1)
        
        # downsampled raster data
        p_src = cv2.pyrDown(
            p,
            dstsize=(
                p.shape[1] // 2,
                p.shape[0] // 2))
        with rasterio.open(
            filename_small,
            mode='w',
            height=p_src.shape[0],
            width=p_src.shape[1],
            count=1,
            dtype=p_src.dtype,
            driver='GTiff',
            crs=crs,
            transform=t
        ) as dataset_utm:
            dataset_utm.write(p_src, 1)
        count = count + 1
    print(f'Discarded {discard} patches')
    return patches, mpp



def compute_rivers(tiff_image):
    grid = Grid.from_raster(str(tiff_image), data_name='dem')
    depressions = grid.detect_depressions('dem')

    grid.fill_depressions(data='dem', out_name='flooded_dem')
    flats = grid.detect_flats('flooded_dem')
    grid.resolve_flats(data='flooded_dem', out_name='inflated_dem')

    # Compute flow direction based on corrected DEM
    dirmap = (64, 128, 1, 2, 4, 8, 16, 32)
    grid.flowdir(data='inflated_dem', out_name='dir', dirmap=dirmap)
    # Compute flow accumulation based on computed flow direction
    grid.accumulation(data='dir', out_name='acc', dirmap=dirmap)
    downsampled_rivers = np.log(grid.view('acc') + 1)

    # remove small flats
    flats = binary_opening(flats).astype(dtype=np.uint8)*255

    # flats = binary_closing(skeletonize(flats)).astype(dtype=np.uint8)*255

    _, upsampled_flats = cv2.threshold(cv2.pyrUp(flats), 127, 255, cv2.THRESH_BINARY)

    downsampled_rivers = (downsampled_rivers - np.amin(downsampled_rivers)) / \
        (np.amax(downsampled_rivers) - np.amin(downsampled_rivers))
    
    downsampled_rivers = np.array(downsampled_rivers * 255, dtype=np.uint8)
    _, thresholded_river = cv2.threshold(downsampled_rivers, 127, 255, cv2.THRESH_BINARY)
    thresholded_river[thresholded_river == 255] = 1
    
    thresholded_river = cv2.pyrUp(thresholded_river)
    skeletonized_rivers = binary_closing(skeletonize(thresholded_river)).astype(dtype=np.uint8)*255

    # thicken lines
    # kernel = np.ones((3,3), np.uint8)
    # skeletonized_rivers =  cv2.dilate(skeletonized_rivers, kernel)

    return np.expand_dims(skeletonized_rivers, axis=-1), np.expand_dims(upsampled_flats, axis=-1)

def compute_ridges_b(tiff_image):
    grid = Grid.from_raster(str(tiff_image), data_name='dem')
    raw_data = grid.dem
    grid.dem = grid.dem.max() - grid.dem
    depressions = grid.detect_depressions('dem')

    grid.fill_depressions(data='dem', out_name='flooded_dem')
    flats = grid.detect_flats('flooded_dem')
    grid.resolve_flats(data='flooded_dem', out_name='inflated_dem')

    # Compute flow direction based on corrected DEM
    dirmap = (64, 128, 1, 2, 4, 8, 16, 32)
    grid.flowdir(data='inflated_dem', out_name='dir', dirmap=dirmap)
    # Compute flow accumulation based on computed flow direction
    grid.accumulation(data='dir', out_name='acc', dirmap=dirmap)
    downsampled_ridges = np.log(grid.view('acc') + 1)

    # remove small flats
    # flats = binary_opening(flats).astype(dtype=np.uint8)*255

    depressions = binary_closing(skeletonize(depressions)).astype(dtype=np.uint8)*255
    
    _, upsampled_depressions = cv2.threshold(cv2.pyrUp(depressions), 127, 255, cv2.THRESH_BINARY)

    downsampled_ridges = (downsampled_ridges - np.amin(downsampled_ridges)) / \
        (np.amax(downsampled_ridges) - np.amin(downsampled_ridges))
    
    downsampled_ridges = np.array(downsampled_ridges * 255, dtype=np.uint8)
    _, thresholded_ridges = cv2.threshold(downsampled_ridges, 150, 255, cv2.THRESH_BINARY)
    thresholded_ridges[thresholded_ridges == 255] = 1
    
    thresholded_ridges = cv2.pyrUp(thresholded_ridges)
    skeletonized_ridges = binary_closing(skeletonize(thresholded_ridges)).astype(dtype=np.uint8)*255

    # thicken lines
    # kernel = np.ones((2,2), np.uint8)
    # skeletonized_ridges =  cv2.dilate(skeletonized_ridges, kernel)

    #  detect peaks on non inverted elevation data
    updem = cv2.pyrUp(raw_data)
    neighborhood_size = 50
    threshold = 50
    data_max = filters.maximum_filter(updem, neighborhood_size)
    maxima = (updem == data_max)
    data_min = filters.minimum_filter(updem, neighborhood_size)
    diff = ((np.abs(data_max - data_min)) > threshold)
    maxima[diff == 0] = 0
    # print(maxima.shape)
    # print(skeletonized_ridges.shape)

    return np.expand_dims(skeletonized_ridges, axis=-1), \
        np.expand_dims(maxima, axis=-1)

def compute_ridges(tiff_image):
    grid = Grid.from_raster(str(tiff_image), data_name='dem')
    grid.dem = grid.dem.max() - grid.dem
    peaks = grid.detect_depressions('dem')
    grid.fill_depressions(data='dem', out_name='flooded_dem')
    flats = grid.detect_flats('flooded_dem')
    grid.resolve_flats(data='flooded_dem', out_name='inflated_dem')

    # Compute flow direction based on corrected DEM
    dirmap = (64, 128, 1, 2, 4, 8, 16, 32)
    grid.flowdir(data='inflated_dem', out_name='dir', dirmap=dirmap)
    # Compute flow accumulation based on computed flow direction
    grid.accumulation(data='dir', out_name='acc', dirmap=dirmap)
    downsampled_ridges = np.log(grid.view('acc') + 1)
    upsampled_peaks = cv2.pyrUp(np.array(peaks, dtype=np.uint8))
    upsampled_ridges = cv2.pyrUp(downsampled_ridges)
    upsampled_ridges = (upsampled_ridges - np.amin(upsampled_ridges)) / \
        (np.amax(upsampled_ridges) - np.amin(upsampled_ridges))
    upsampled_ridges = np.array(upsampled_ridges * 255, dtype=np.uint8)
    _, thresholded_ridges = cv2.threshold(upsampled_ridges, 150, 255, cv2.THRESH_BINARY)
    thresholded_ridges[thresholded_ridges == 255] = 1
    skeletonized_ridges = skeletonize(thresholded_ridges)

    return np.expand_dims(skeletonized_ridges, axis=-1), np.expand_dims(upsampled_peaks, axis=-1)

def extract_sketches(args):
    filenames, outputfile = args
    # create flow and sketch data from downsampled(blurred) terrain images. Then upsample data to match original terrain
    height_maps = []
    sketch_maps = []
    for raster_tile, hq_tile in filenames:
        print(Path(raster_tile).name)
        rivers, basins = compute_rivers(raster_tile)
        ridges, maxima = compute_ridges_b(raster_tile)

        # # basins preview
        # print('basins')
        # fig = plt.figure(figsize=(12, 8))
        # ax = fig.add_subplot(111)
        # ax.matshow(basins)
        # print(np.min(basins))
        # print(np.max(basins))
        # ax.matshow(basins)
        # fig.savefig('data/basins_preview.png', dpi=500)

        # # river preview
        # print('river')
        # fig = plt.figure(figsize=(12, 8))
        # ax = fig.add_subplot(111)
        # create_utm_preview('tile', raster_tile)
        # ax.matshow(rivers)
        # print(np.min(rivers))
        # print(np.max(rivers))
        # ax.matshow(rivers)
        # fig.savefig('data/rivers_preview.png', dpi=500)

        # # ridges preview
        # print('ridges')
        # fig = plt.figure(figsize=(12, 8))
        # ax = fig.add_subplot(111)
        # ax.matshow(ridges)
        # print(np.min(ridges))
        # print(np.max(ridges))
        # ax.matshow(ridges)
        # fig.savefig('data/ridges_preview.png', dpi=500)

        # # maxima preview
        # print('maxima')
        # fig = plt.figure(figsize=(12, 8))
        # ax = fig.add_subplot(111)
        # create_utm_preview('tile', raster_tile)
        # ax.matshow(maxima)
        # print(np.min(maxima))
        # print(np.max(maxima))
        # ax.matshow(maxima)
        # fig.savefig('data/maxima_preview.png', dpi=500)
        # break

        with rasterio.open(hq_tile) as dataset:
            height_map = np.array(dataset.read(1), dtype=np.float32)
        
        height_map = np.expand_dims(height_map, axis=-1)
        height_max = np.amax(height_map)
        height_min = np.amin(height_map)
        height_map = (height_map - height_min) / \
            (height_max - height_min)
        height_map = height_map * 2 - 1

        # build elevation hints
        labeled, num_objects = ndimage.label(maxima)
        slices = ndimage.find_objects(labeled)
        maxima_b = np.ones(maxima.shape) * -1
        # print(slices)
        x, y = [], []
        for dy,dx,_ in slices:
            x_center = (dx.start + dx.stop - 1)/2
            x.append(int(x_center))
            y_center = (dy.start + dy.stop - 1)/2    
            y.append(int(y_center))

        for X, Y in zip(x, y):
            maxima_b[Y, X] = height_map[Y, X]
        maxima_b = np.array((maxima_b + 1) * 127.5, dtype=np.uint8)
        kernel = np.ones((15,15), np.uint8)
        maxima_b =  cv2.dilate(maxima_b, kernel)
        maxima_b = np.expand_dims(maxima_b, axis=-1)
        # print(ridges.shape)
        # print(rivers.shape)
        # print(maxima_b.shape)

        # # maxima preview
        # print('maxima')
        # fig = plt.figure(figsize=(12, 8))
        # ax = fig.add_subplot(111)
        # create_utm_preview('tile', raster_tile)
        # ax.matshow(maxima_b)
        # print(np.min(maxima_b))
        # print(np.max(maxima_b))
        # ax.matshow(maxima_b)
        # fig.savefig('data/maxima_preview.png', dpi=500)
        # break

        sketch_map = np.stack((ridges, rivers, maxima_b), axis=2)
        sketch_map = np.squeeze(sketch_map, axis=-1)
        # print(sketch_map.shape)
        height_maps.append(height_map)
        sketch_maps.append(sketch_map)
        # create_utm_preview('tile', raster_tile)
    print(f'Total {len(height_maps)} height maps')
    training_output = np.stack(height_maps)
    training_input = np.stack(sketch_maps)
    print(training_output.shape)
    np.savez(outputfile, x=training_input, y=training_output)


if __name__ == "__main__":
    # (left, bottom, right, top)
    data_sets = {
        ('example2', (-90.000672, 45.998852, -87.998534, 46.999431)),
        ('data0', (-123.279303, 37.132537, -121.105405, 39.724539))
    }
    tile_dir = 'data/tiles'

    # refresh tiles
    regen_tiles = True
    force_download = True

    for data_name, bounds in data_sets:
        data_file = f'data/raw/{data_name}.tif'
        utm_data_file = f'data/raw/{data_name}_utm.tif'

        if not os.path.exists(utm_data_file) or force_download:
            if not os.path.exists(data_file) or force_download:
                download_dem_data(bounds, data_file)

            dataset = rasterio.open(data_file)
            print(f"Downloaded Dataset bounds: {dataset.bounds}")
            dem_array, dst_transform, dst_crs, xy_mm = to_utm(dataset, bounds)
            print(f"Estimated Dataset bounds: {xy_mm}")
            with rasterio.open(
                utm_data_file,
                mode='w',
                height=dem_array.shape[0],
                width=dem_array.shape[1],
                count=1,
                dtype=dem_array.dtype,
                driver='GTiff',
                crs=dst_crs,
                transform=dst_transform
            ) as dataset_utm:
                dataset_utm.write(dem_array, 1)
            
            create_utm_preview(data_name, utm_data_file)
        
        if regen_tiles:
            _patches, mpp = extract_patches_from_raster(data_name, utm_data_file, tile_dir, (1200, 1200), 500)
            print(f'Got {len(_patches)} patches.')

    # Path('./data/tiles/downsampled').glob('*.tif')
    


