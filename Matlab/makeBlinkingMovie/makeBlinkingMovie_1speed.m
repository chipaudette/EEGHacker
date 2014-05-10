% Created: Chip Audette, http://eeghacker.blogspot.com
% Date: May 2014
% Purpose: Create blinking movies at different rates
% Platform:  Matlab 7.1 on Windows XP
% License: The MIT License (MIT)

% Choose aspect ratio for the movie
width_vs_height = 16/9;

%make color map for the movie
map = ones(256,3); %create color map, all white
map(1,:) = 1; %modify color map by making first entry as black

% make the black and white frames
hblock = 64;
wblock = round(64*width_vs_height);
nblock=1;
npix_w=wblock*nblock;
npix_h=hblock*nblock;
X1 = uint8(ones(npix_h,npix_w));
X2 = uint8(zeros(npix_h,npix_w));
    

% view the two frames
figure;
try;setFigureWide;catch;end
subplot(1,2,1);
try;imshow(X1,map);catch;image(X1);colormap(map);end
title('Frame 1');
axis equal;axis tight;
subplot(1,2,2);
try;imshow(X2,map);catch;image(X2);end
title('Frame 2');
axis equal;axis tight;

% loop to make individual movies at the desired blink rates
desired_rates_Hz = [1:20];  %this sets the white-to-white blink rate
for Irate = 1:length(desired_rates_Hz)
    desired_rate_Hz = desired_rates_Hz(Irate);
    
    %fabricate series of movies
    desired_rate_Hz = desired_rates_Hz(Irate);  %for complete cycle of X1 and X2 together
    desired_duration_sec = 15;
    n_frames = round(desired_rate_Hz * desired_duration_sec)
    clear M
    for I = 1:n_frames
        M((I-1)*2+1)=im2frame(X1,map);
        M((I-1)*2+2)=im2frame(X2,map);
    end
    
    % watch movie
    %figure;setFigureTallWide;set(gcf,'DoubleBuffer','on');movie(M,1,desired_rate_Hz*2);
    
    % save movie
    outpname=['Movies_' num2str(nblock) 'block\'];
    try;mkdir(outpname);catch;end;
    outfname = [outpname 'checkerboard' num2str(nblock) '_rate' num2str(desired_rate_Hz) 'HzX2'];
    if (1)
    	%write as AVI
        outfname = [outfname '.avi'];
        disp(['Writing to ' outfname]);
        movie2avi(M,outfname,'Compression','none','FPS',desired_rate_Hz*2);
    else
    	%write as WMV...doesn't seem to work anymore.  So, I'll use the AVI and
        %then use windows movie maker to convert to WMV
        clear video2
        video2.width=size(M(1).cdata,2);
        video2.height=size(M(1).cdata,1);
        video2.frames=M;
        video2.rate=desired_rate_Hz*2;
        video2.times=[0:1:length(M)-1] /video2.rate;
        
        outfname = [outfname '.wmv'];
        disp(['Writing to ' outfname]);
        mmwrite(outfname,video2);
    end
end
