fp = fopen('orig64_neg.bin', 'rb');

width = fread(fp, 1, 'uint32');
height = fread(fp, 1, 'uint32');

figure; imshow(uint8(reshape(fread(fp), [width height])'));
fclose(fp);