datestr = '2014-08-17';
pname = 'Data\';
fname = '05-testSig_1x_Fast.bin';
fname = '06-normalInput.bin';
%fname = '07-capture_bot_bot.bin';
fname = '08-capture_top_top.bin';
%fname = '09-capture_bot_top.bin';
%fname = '10-capture_top_bot.bin';

nbytes_per_packet = 36;
fs = 250;

% read data
fid = fopen([pname fname],'r');
data = uint8(fread(fid,'uint8'));
fclose(fid);
disp(['read ' num2str(length(data)) ' bytes']);

%% find repetition rate
startbyte = hex2dec('A0');
startbyte = 160;
%endbyte = hex2dec('C0');
% I=find(data==startbyte);
% dI=diff(I);
% J=find(dI > 2);
% nbytes_per_packet = median(dI(J))

%% parse the data
origdata=data;
outdata=uint8(zeros(ceil(length(data)/nbytes_per_packet),nbytes_per_packet));
ind_out = 0;
ind = 1;done=0;
while ~done
    if (data(ind)==startbyte)
        ind_start_next = ind+nbytes_per_packet;
        if (ind_start_next < length(data)) | (length(data) == ind_start_next-1)
            if ((length(data) == ind_start_next-1) | (data(ind+nbytes_per_packet)==startbyte))
                ind_out=ind_out+1;
                inds = ind+[0:(nbytes_per_packet-1)];
                outdata(ind_out,:) = data(inds);
                ind = ind+nbytes_per_packet-1;
            end
        end
    end
    ind = ind+1;
    if (ind >= length(data))
        done=1;
    end
end
data = outdata(1:ind_out,:);
clear outdata

%remove last column if 192
% if all(data(:,end)==192)
%     disp(['Removing last column of data because it is all 192']);
%     data = data(:,1:(end-1));
% end


%% create int32 from the 24-bit values
packet_counter_ind = 3;
packet_counter = double(data(:,packet_counter_ind));
%start_ind = packet_counter_ind+1;;end_ind = 8*4+start_ind-1;
%inds = [start_ind:4:end_ind];
fulldata = int32(zeros(size(data,1),8));

    %MSByte first
    for Irow=1:size(fulldata,1)
        for Icol=1:size(fulldata,2)
            inds = packet_counter_ind+(Icol-1)*4+[1:4];
            bytes = data(Irow,inds);
            if 1
                bytes = bytes(end:-1:1);  %switch byte order
            end
            val = typecast(bytes,'int32');
            fulldata(Irow,Icol) = val;
        end
    end
    MSB_txt = 'MSB';


fulldata=double(fulldata);

%% filter the data
fulldata = fulldata - ones(size(fulldata,1),1)*mean(fulldata);
[b,a]=butter(1,[55 65]/(fs/2),'stop');
fulldata = filter(b,a,fulldata);
[b,a]=butter(2,65/(fs/2));
fulldata = filter(b,a,fulldata);




%% plot
% figure;
% setFigureTallestWidest;
% nchan=size(data,2);
% ncol=7;
% nrow = ceil(nchan/ncol);
% for Idata=1:nchan
%     subplotTightBorder(nrow,ncol,Idata);
%     y = data(:,Idata);
%     x = [1:length(y)];
%     t = x /fs;
%     plot(t,y);
%     xlabel(['Time (sec)']);
%     ylabel(['Signal']);
%     title(['Col ' num2str(Idata) ', Step = ' num2str(nbytes_per_packet)]);
%     xlim([0 size(data,1)/fs]);
%     ylim([0 255]);
% end

%% show interpretted data
figure;setFigureTallWide;
nrow=2;ncol=2;
subplotTightBorder(nrow,ncol,1);
t = [1:length(packet_counter)]/fs;
plot(t,packet_counter);
xlim(t([1 end]));
ylim([0 255]);

subplot(nrow,ncol,2);
plot(t(2:end),diff(packet_counter));
xlim(t([1 end]));
ylim([-1 10]);

for Iplot=1:2
    subplot(nrow,ncol,ncol+Iplot);
    y = fulldata(:,Idata);
    y_uV = y * 4.5 / (2^23-1) / 24 * 1e6;
    plot(t,y_uV);
    title(['OpenBCI V3: Channel ' num2str(Idata)]);
    xlim(t([1 end]));
    %ylim([-2^31 2^31]);
    xlabel('Time (sec)');
    ylabel('uV');
    ylim(500*[-1 1]+mean(y_uV));
    
    if (Iplot==2)
        xlim([1 4]);
    end
    
        h=weaText({datestr;fname},3);
    set(h,'Interpreter','none');
end


