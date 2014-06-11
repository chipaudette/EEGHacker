% Created: Chip Audette, http://eeghacker.blogspot.com
% Date: May 2014
% Purpose: Create movies with two simultaneous blink rates,
%    one blink rate on the left and another blink rate on
%    the right.
% Platform:  Matlab 7.1 on Windows XP
% License: The MIT License (MIT)

%make black and white frames
if (1)
    map = ones(256,3); %all white
    map(1,:) = 0; %black

    % make the two frames...all black and all white
    width_vs_height = 16/9;
    hblock = 64;
    wblock = round(hblock*width_vs_height/2);
    nblock=1;
    npix_w=wblock*nblock;
    npix_h=hblock*nblock;
    Xwhite = ones(npix_h,npix_w);
    Xblack = ~Xwhite;
    
    %put a dot in the middle of the white one
    np=1;
    xcenter = round(wblock/2)-1;
    hcenter = round(hblock/2)-1;
    Xwhite(hcenter-2,xcenter)=0;
    Xwhite(hcenter,xcenter+[-2 2])=0;
    Xwhite(hcenter+2,xcenter)=0;
    
    %convert to "byte"
    Xwhite = uint8(Xwhite);
    Xblack = uint8(Xblack);
end

%choose the blinking rate
switch 3
    case 1
        overall_toggle_Hz = 60;
        nleft = 3;
        nright = 4;
    case 2
        overall_toggle_Hz = 30;
        nleft = 2;
        nright = 3;
    case 3
        overall_toggle_Hz = 20;
        nleft = 2;
        nright = 3;        
end
left_toggle_Hz = overall_toggle_Hz/nleft;
right_toggle_Hz = overall_toggle_Hz/nright;

%set the movie diration
if (0)
    %I did this branch for my first successful 2-speed EEG Hacker post
    desired_duration_sec = 20;
    n_swap = 4;  % how many times to swap the left and right sides
else
    %I did this branch to produce actual useful movies for my BCI
    desired_duration_sec = 30;
    n_swap = 1;  % don't switch sides
end
nframes = round(desired_duration_sec*overall_toggle_Hz);

%create the movie
n_frames = round(overall_toggle_Hz * desired_duration_sec)
M=[];
left_state=0;right_state=0;
for Iswap=1:n_swap
    for I = 1:n_frames
        %change states (ie, change which side the movie is on)
        if (rem(I-1,nleft)==0);
            left_state = ~left_state;
        end
        if(rem(I-1,nright)==0);
            right_state = ~right_state;
        end

        %choose which is the left and which is the right image
        XL = Xblack; XR = Xblack;
        if (left_state), XL = Xwhite;end
        if (right_state), XR = Xwhite; end;

        %build the composite left+right image
        if (rem(Iswap,2)==1)
            X = [XL XR];
        else
            X = [XR XL];
        end

        %save to movie
        if isempty(M)
            M = im2frame(X,map);
        else
            M(end+1) = im2frame(X,map);
        end

    end
end

% watch movie
%figure;setFigureTallWide;set(gcf,'DoubleBuffer','on');movie(M,1,desired_rate_Hz*2);

% save movie as AVI
outpname=['TwoSpeedMovie\'];
try;mkdir(outpname);catch;end;
outfname = [outpname 'Block' num2str(nblock) ...
    '_' num2str(left_toggle_Hz,3) 'HzToggle' ...
    '_' num2str(right_toggle_Hz,3) 'HzToggle'];

outfname = [outfname '.avi'];
disp(['Writing to ' outfname]);
try;eval(['!del "' outfname '"']);catch;end;   %for WINDOWS!
movie2avi(M,outfname,'Compression','none','FPS',overall_toggle_Hz);


