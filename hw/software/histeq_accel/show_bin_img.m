fp = fopen('low_contrast64_hist.bin', 'rb');

width = fread(fp, 1, 'uint32');
height = fread(fp, 1, 'uint32');

figure; imshow(uint8(reshape(fread(fp), [width height])'));
fclose(fp);