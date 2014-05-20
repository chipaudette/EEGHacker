
%given_amp_counts = 4.5/2.4e-3;


pname = 'SavedData\';
t_baseline_sec=[];notch60=0;notch70=0;notch120=0;
s_fname=[];
switch 20
     case 2
        fname = 'openBCI_raw_2014-04-05_16-30-54.txt';nchan=1;
        t_sig_sec = [12 21.25];
    case 3
        %ECG Electrodes
        fname = 'openBCI_raw_2014-04-05_16-36-34_ECGelec_countBackBy3_noFilt.txt';nchan=1;
        t_sig_sec = [33.5 50;86.2 91.9];
    case 4
        fname = 'openBCI_raw_2014-04-05_16-39-58.txt'; nchan=1;
        t_sig_sec = [59.9 75; 131.7 138.2];
    case 5
        %EEG Electrodes
        fname = 'openBCI_raw_2014-04-05_17-13-48_GoldCup_countBackBy3_afterLastAlpa.txt';nchan=1;
        t_sig_sec = [192.3 218.4; 261.1 267.3];
    case 10
        fname = '10-openBCI_raw_2014-04-19_10-23-36_EyesClosed_8secBreaths.txt';nchan=1;
        if (1)
            t_sig_sec = [285 336];
            t_baseline_sec = [187 213];
        else
            t_sig_sec = [215 285];
            t_baseline_sec = [345 400];
        end
        notch60=1;notch70=1;notch120=1;
    case 12
        fname = '12-openBCI_raw_2014-04-19_10-40-38_countbackwardsby3.txt';nchan=1;
        t_sig_sec = [90 130];
        %t_baseline_sec = [40 55];
        t_baseline_sec = [155 179];
        notch60=1;
    case 13
        fname = '13-openBCI_raw_2014-04-19_10-54-51_bothOnForehead_countback.txt';nchan=1;
        %t_sig_sec = [63 84];
        t_sig_sec = [100 113];
        %t_baseline_sec = [47 55];
        t_baseline_sec = [65 80];
        notch60=1;
    case 20
        fname = 'openBCI_raw_2014-04-23_06-52-48_Breakfast_Birds_CountBack.txt';nchan=1;
        t_sig_sec = [29*60+40.4 30*60+54.4];
        t_baseline_sec = [27*60+14.7 27*60+44.9];
        notch60=1;
end
if isempty(s_fname);s_fname = [fname(1:3) fname(36:end-4)];end;
%fname = 'openBCI_raw_2014-04-05_16-36-34_ECGelec_countBackBy3_noFilt.txt';nchan=2;
%fname = 'openBCI_raw_2014-04-05_17-13-48_GoldCup_countBackBy3_afterLastAlpa.txt';nchan=2;
scale_fac_volts_count=2.23e-8;

current_A = 6e-9;


%% load data
data_uV = load([pname fname]);  %loads data as microvolts
count = data_uV(:,1);  %first column is a packet counter
data_uV = data_uV(:,[1:(nchan+1)];
fs = 250;
data_V = data_uV * 1e-6; %other columns are data
clear data_uV;

%% filter data around the test tone
data_V = data_V - ones(size(data_V,1),1)*mean(data_V);
if notch60
    [b,a]=butter(2,[56.5 63.5]/(fs/2),'stop');
    data_V = filter(b,a,data_V);  %apply notch filter
end
if notch120
    [b,a]=butter(2,[115 122]/(fs/2),'stop');
    data_V = filter(b,a,data_V);  %apply notch filter
end
if notch70
    [b,a]=butter(2,[66.5 73.5]/(fs/2),'stop');
    data_V = filter(b,a,data_V);  %apply notch filter
end

%filter to get the beta
bp_Hz = [22 100];
%bp_Hz = [12 20];
%bp_Hz = [7 15];
%bp_Hz = [3 30];
Nfir = 2*round(0.5*fs);  %ensure an even number
[b_bp,a_bp]=weaFIR(Nfir,(bp_Hz)/(fs/2));
fdata_V = filter(b_bp,a_bp,data_V);
fdata_V = [fdata_V(Nfir/2+1:end,:);zeros(Nfir/2,size(fdata_V,2))];  %remove latency

%get amplitude of beta
ave_sec = 2;
if (0)
    N = 2*round(0.5*ave_sec*fs); %make even
    b_ave = 1/N*ones(N,1);
    a_ave = 1;
else
    N = 2*ave_sec*fs;
    [b_ave,a_ave]=fir1(N,(1/ave_sec)/(fs/2));
end
rms_V = sqrt(filter(b_ave,a_ave,fdata_V.^2));
rms_V = [rms_V(N/2+1:end,:);zeros(N/2,size(rms_V,2))];  %remove filter latency

%assess SNR
rms_dBuV = 10*log10((rms_V*1e6).^2);
noise_inds = round(t_baseline_sec.*fs);
noise_dBuV =10*log10(nanmean(10.^(0.1*rms_dBuV(noise_inds(1):noise_inds(2),:))));
snr_dB = rms_dBuV - ones(size(rms_dBuV,1),1)*noise_dBuV;

%% repeat analysis after whitening
Nwhite=512/2;overlap = 1-1/16;plots=0;
[pD,wT,f]=windowedFFTPlot_spectragram(data_V*1e6,Nwhite,overlap,fs,plots);
wT = wT + (Nwhite/2)/fs;
inds = find((wT-(Nwhite/2)/fs >= t_baseline_sec(1,1)) & (wT+(Nwhite/2)/fs <= t_baseline_sec(1,2)));
pD_noise = mean(pD(:,inds)')';
%Iinband=find((f >= bp_Hz(1)) & (f <= bp_Hz(2)));
Iinband = find((f >= 2) & (f <=bp_Hz(2)));
mean_inband_pD = median(pD_noise(Iinband));
pD_noise = pD_noise / mean_inband_pD;
Ioutband=find((f < 2) | (f > bp_Hz(2)));
pD_noise(Ioutband) = 1;
inv_filt_amp = 1./sqrt(pD_noise);
if notch60; I=find((f>55) & (f<65));inv_filt_amp(I)=0;end;%cut
if notch70; I=find((f>65) & (f<75));inv_filt_amp(I)=0;end;%cut
if notch120; I=find(f>115); inv_filt_amp(I)=0;end;%cut
inv_filt_amp = [inv_filt_amp(:);conj(inv_filt_amp(end-1:-1:2))];%recreate negative freq space
b_white = real(ifft(inv_filt_amp)); %make the filter coefficients
b_white=[b_white(Nwhite/2+1:end); b_white(1:Nwhite/2)];
b_white=hanning(Nwhite).*b_white;
a_white = 1;
wdata_V = filter(b_white,a_white,data_V); %apply the whitening filter
wdata_V = [wdata_V(Nwhite/2+1:end,:);zeros(Nwhite/2,size(wdata_V,2))];

%continue processing like normal
fwdata_V = filter(b_bp,a_bp,wdata_V);
ffdata_V = [fwdata_V(Nfir/2+1:end,:);zeros(Nfir/2,size(fwdata_V,2))];  %remove latency
wrms_V = sqrt(filter(b_ave,a_ave,ffdata_V.^2));
wrms_V = [wrms_V(N/2+1:end,:);zeros(N/2,size(wrms_V,2))];  %remove filter latency
wrms_dBuV = 10*log10((wrms_V*1e6).^2);
wnoise_dBuV =10*log10(nanmean(10.^(0.1*wrms_dBuV(noise_inds(1):noise_inds(2),:))));
wsnr_dB = wrms_dBuV - ones(size(wrms_dBuV,1),1)*wnoise_dBuV;




%% quantify data
mean_data_V = [];
std_data_V = [];
std_noise_data_V=[];
mean_snr_dB=[];
for Idata=1:size(t_sig_sec);
    inds = round(t_sig_sec(Idata,:)*fs);
    inds = [max([1 inds(1)]) min([size(fdata_V,1) inds(2)])];
    inds = [inds(1):inds(2)];
    %mean_data_V(Idata,:) = mean(fdata_V(inds,:));
    std_data_V(Idata,1) = nanstd(fdata_V(inds,:));
    mean_snr_dB(Idata,1) = 10*log10(nanmean(10.^(0.1*(snr_dB(inds,:)))));
    std_data_V(Idata,2) = nanstd(fwdata_V(inds,:));
    mean_snr_dB(Idata,2) = 10*log10(nanmean(10.^(0.1*(wsnr_dB(inds,:)))));
    %imp_data_Ohm(Idata,:) =  std_data_V(Idata,:)*sqrt(2) / current_A;
    
    inds = round(t_baseline_sec(Idata,:)*fs);
    inds = [max([1 inds(1)]) min([size(fdata_V,1) inds(2)])];
    inds = [inds(1):inds(2)]; 
    std_noise_data_V(Idata,1) = nanstd(fdata_V(inds,:));
    std_noise_data_V(Idata,2) = nanstd(fwdata_V(inds,:));
end


%% simple plot
Ichan=1;

t_sec = ([1:size(data_V,1)]-1)/fs;

figure;setFigureTallestPartWide;ax=[];
%set(gcf,'position',[75          45        1295         776]);


subplot(3,1,1);
N=512/2;overlap = 1-1/2;plots=0;
%Ichan=1;
[pD,wT,f]=windowedFFTPlot_spectragram(data_V(:,Ichan)*1e6,N,overlap,fs,plots);
wT = wT + (N/2)/fs;
imagesc(wT,f,10*log10(pD));
set(gca,'Ydir','normal');
%if (Idata > 6); xlabel('Time (sec)');end
ylabel('Frequency (Hz)');
%title([fname ', Channel ' num2str(Ichan)],'interpreter','none');
title([s_fname ', Channel ' num2str(nchan)],'interpreter','none')
%set(gca,'Clim',+8+[-24 0]+10*log10(256)-10*log10(N));
set(gca,'Clim',+20+[-40 0]+10*log10(256)-10*log10(N));
ylim([0 100]);flim=ylim;
xlim(t_sec([1 end]));xl=xlim;
xlabel('Time (sec)');
cl=get(gca,'Clim');h=weaText(['[' num2str(cl(1),2) ' ' num2str(cl(2),2) '] dB re: 1 uV'],2);
set(h,'BackgroundColor','white');
ax(end+1)=gca;

hold on;
xl=xlim;
plot(xl,bp_Hz(1)*[1 1],'w--','linewidth',2);
plot(xl,bp_Hz(2)*[1 1],'w--','linewidth',2);
hold off

subplot(3,1,2);
%plot(t_sec,[rms_V(:) wrms_V(:)]*1e6,'linewidth',2);
plot(t_sec,rms_V(:)*1e6,'linewidth',2);
ylim([0 6]);
if (any(std_data_V(:,1)*1e6 > 5.5)); ylim([0 10]);end;
ylabel(['RMS (uV) over ' num2str(bp_Hz(1)) '-' num2str(bp_Hz(2)) ' Hz']);
xlabel(['Time (sec)']);
xlim(xl);
%legend('BP Only','Whitened+BP',2);
ax(end+1)=gca;

subplot(3,1,3);
plot(t_sec,[snr_dB(:) wsnr_dB(:)],'linewidth',2);
ylabel(['SNR (dB) over ' num2str(bp_Hz(1)) '-' num2str(bp_Hz(2)) ' Hz']);
xlabel(['Time (sec)']);
xlim(xl);
hold on;plot(xlim,[0 0],'k--','linewidth',2);hold off
legend('Raw','Whitened',3);
ax(end+1)=gca;
%ylim(bp_Hz+[-2 2]);
ylim([-5 15]);

for Iplot=1:3
    subplot(3,1,Iplot);
    for Idata=1:size(t_sig_sec,1);
        hold on;
        c1 = 'g';c2='r';
        if (Iplot==1);c1='w';c2='w';end
        yl=ylim;plot(t_sig_sec(Idata,1)*[1 1],yl,[c1 '--'],'linewidth',2);
        plot(t_sig_sec(Idata,2)*[1 1],yl,[c2 '--'],'linewidth',2);hold off
        hold off

        if ~isempty(t_baseline_sec)
            hold on;
            yl=ylim;plot(t_baseline_sec(Idata,1)*[1 1],yl,[c1 ':'],'linewidth',2);
            plot(t_baseline_sec(Idata,2)*[1 1],yl,[c2 ':'],'linewidth',2);hold off
            hold off
        end

        yl=ylim;
        if (Iplot==2)
            for Itype=1:2
                switch Itype
                    case 1
                        val = std_data_V(Idata,1)*1e6;
                        x = t_sig_sec(Idata,:);
                    case 2
                        val = std_noise_data_V(Idata,1)*1e6;
                        x = t_baseline_sec(Idata,:);

                end

                hold on;plot(x,val*[1 1],'k:','linewidth',2);hold off
                h=text(mean(x),val+0.15*diff(yl),{num2str(val,3); 'uVrms'});
                set(h,'HorizontalAlignment','center','VerticalAlignment','bottom','backgroundcolor','white');
            end
            
%             val = std_data_V(Idata,2)*1e6;
%             hold on;plot(t_sig_sec,val*[1 1],'k:','linewidth',2);hold off
%             h=text(mean(t_sig_sec(Idata,:)),val+0.1*diff(yl),{num2str(val,3); 'uVrms'});
%             set(h,'HorizontalAlignment','center','VerticalAlignment','bottom','backgroundcolor','white');
        end
        if (Iplot==3)
            val = mean_snr_dB(Idata,1);
            hold on;plot(t_sig_sec,val*[1 1],'k:','linewidth',2);hold off
            h=text(mean(t_sig_sec(Idata,:)),val-0.1*diff(yl),{num2str(val,3); 'dB'});
            set(h,'HorizontalAlignment','center','VerticalAlignment','top','backgroundcolor','white');
            val = mean_snr_dB(Idata,2);
            hold on;plot(t_sig_sec,val*[1 1],'k:','linewidth',2);hold off
            h=text(mean(t_sig_sec(Idata,:)),val+0.1*diff(yl),{num2str(val,3); 'dB'});
            set(h,'HorizontalAlignment','center','VerticalAlignment','bottom','backgroundcolor','white');
        end
    end
end

linkaxes(ax,'x');


%% add spectrum
 figure;setFigureTallWide;
 for Idata=1:2
    switch Idata
        case 1
            data = data_V(:,1)*1e6;
            sname='';
        case 2
            data = wdata_V(:,1)*1e6;
            sname='(whitened)';
    end
    N=512/2;overlap = 1-1/16;plots=0;
    %Ichan=1;
    [pD,wT,f]=windowedFFTPlot_spectragram(data,N,overlap,fs,plots);
    wT = wT + (N/2)/fs;

    if notch60; I=find((f>=55) & (f<=65));pD(I,:)=NaN;end;%cut
    if notch70; I=find((f>=65) & (f<=75));pD(I,:)=NaN;end;%cut
    if notch120; I=find(f>=115); pD(I,:)=NaN;end;%cut

    inds = find((wT-(N/2)/fs >= t_sig_sec(1,1)) & (wT+(N/2)/fs <= t_sig_sec(1,2)));
    pD_sig = nanmean(pD(:,inds)')';
    inds = find((wT-(N/2)/fs >= t_baseline_sec(1,1)) & (wT+(N/2)/fs <= t_baseline_sec(1,2)));
    pD_noise = nanmean(pD(:,inds)')';

    snr = pD_sig./pD_noise;
    snr_dB = 10*log10(snr);
    inds_inband = find((f>=bp_Hz(1)) & (f<=bp_Hz(2)));
    sig_uV = sqrt(nansum(pD_sig(inds_inband)));
    noise_uV = sqrt(nansum(pD_noise(inds_inband)));
    total_snr_dB = 20*log10(sig_uV/noise_uV);
    

    subplot(2,2,Idata)
    plot(f,sqrt([pD_sig pD_noise]),'linewidth',3);
  
    xlabel(['Frequency (Hz)']);
    ylabel(['RMS (uV) per Bin']);
    ylim([0 0.8]);
    if (Idata==2);
        ylim([0.05 10]);
        set(gca,'Yscale','log');
        yl=ylim;set(gca,'YTick',[0.1 1 10],'YTickLabel',{'0.1' '1' '10'});
    end
    xlim(flim);
    legend('Concentrating','Eyes Closed');
    title({['Comparing EEG Spectra ' sname];['[' s_fname ']']},'interpreter','none');
    h=weaText({['N = ' num2str(N)];['fs = ' num2str(fs) ' Hz']},3);set(h,'BackgroundColor','white');
    
    if (Idata==1)
        if (notch60)
            x = 60;
            if (notch70)
                x = 0.5*(60+70);
            end
            yl=ylim;
            h=text(x,yl(1)+0.3*diff(yl),'Notched');
            set(h,'Rotation',90,'VerticalAlignment','middle','HorizontalAlignment','center')
        end
    elseif (Idata==2)
        hold on;
        plot(bp_Hz(1)*[1 1],ylim,'k:','linewidth',2);
        plot(bp_Hz(2)*[1 1],ylim,'k:','linewidth',2);
        hold off

        yl=ylim;
        val = sqrt(noise_uV.^2/diff(bp_Hz));text(mean(bp_Hz),0.75*val,[num2str(val,3) ' uVrms'],'verticalalignment','top','horizontalalignment','center');
        val2 = sig_uV / noise_uV;
        val = sqrt(sig_uV.^2/diff(bp_Hz));

        text(mean(bp_Hz),1.25*val,...
            {[num2str(val,3) ' uVrms'];
            [num2str(val2,3) 'x Noise']},'verticalalignment','bottom','horizontalalignment','center');
    end

    subplot(2,2,2+Idata);
    plot(f,snr_dB,'r','linewidth',2);
    hold on;plot(xlim,[0 0],'k--','linewidth',2);hold off;
    %hold on;plot(xlim,[9 9],'k:','linewidth',2);hold off;
    xlabel(['Frequency (Hz)']);
    ylabel(['SNR (dB)']);
    ylim([-3 15]);yl=ylim;set(gca,'YTick',[fliplr([0:-3:yl(1)]) [3:3:yl(2)]]);
    xlim(flim);
    title({['Comparing EEG Spectra ' sname];['[' s_fname ']']},'interpreter','none');
    h=weaText({['N = ' num2str(N)];['fs = ' num2str(fs) ' Hz']},2);
    set(h,'BackgroundColor','white');
    hold on;
    plot(bp_Hz(1)*[1 1],ylim,'k:','linewidth',2);
    plot(bp_Hz(2)*[1 1],ylim,'k:','linewidth',2);
    hold off

    hold on;
    plot(bp_Hz,total_snr_dB*[1 1],'k:','linewidth',2);
    hold off;
    yl=ylim;
    h=text(mean(bp_Hz),total_snr_dB+0.15*diff(yl),[num2str(total_snr_dB,3) ' dB']);
    set(h,'VerticalAlignment','bottom','horizontalAlignment','center');
end
