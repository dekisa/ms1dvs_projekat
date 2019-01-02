fp = fopen('low_contrast64_hist.bin', 'rb');

width1 = fread(fp, 1, 'uint32');
height1 = fread(fp, 1, 'uint32');

K = uint8(reshape(fread(fp), [width1 height1])');

fclose(fp);

figure(1); 
imshow(K,[]);

fp = fopen('low_contrast64.bin', 'rb');

width = fread(fp, 1, 'uint32');
height = fread(fp, 1, 'uint32');

I = uint8(reshape(fread(fp), [width height])');

fclose(fp);

hist = imhist(I);

cumhist = uint8(round(255*(double(cumsum(hist))/(width*height))));

J = intlut(I, cumhist);

figure(2); 
imshow(J,[]);

figure(3); 
imshow(K-J,[]);