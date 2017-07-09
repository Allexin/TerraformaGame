unit uOSAL;

interface
uses Windows, Classes, SysUtils, OpenGL, Graphics;

type
  TSimpleWriteFileBase = class
  public
    Procedure   WriteNullTerminatedString(const s:ansistring);
    Procedure   WriteDelphiString(const s:ansistring);
    Procedure   WriteInteger(i:integer);
    Procedure   WriteFloat(f:single);
    Procedure   WriteByte(b:Byte);
    Procedure   WriteBoolean(b:boolean);

    Procedure   Write(F:Pointer; WriteSize:integer); virtual; abstract;
    Procedure   Flush(); virtual; abstract;
  end;

  TSimpleWriteFile = class(TSimpleWriteFileBase)
  protected
    FFileName:string;
    FFile:cardinal;
    FStream:TMemoryStream;
  public
    Constructor Create(FileName:string; FormInMemoryFirst:boolean = false);
    Destructor  Destroy();override;
    Procedure   Write(F:Pointer; WriteSize:integer); override;
    Procedure   Flush(); override;
  end;

  TSimpleWriteFileBlockMap = class(TSimpleWriteFileBase)
  protected
    FFileName:string;
    FFile:cardinal;

    FData:Pointer;
    FDataMaxSize:integer;
    FDataCurrentSize:integer;
    Procedure Dump();
  public
    Constructor Create(FileName:string; BlockSize:integer = 10485760);
    Destructor  Destroy();override;
    Procedure   Write(F:Pointer; WriteSize:integer); override;
    Procedure   Flush(); override;
  end;

implementation

{ TSimpleWriteFile }

constructor TSimpleWriteFile.Create(FileName: string; FormInMemoryFirst:boolean = false);
begin
  FFileName:=FileName;

  FStream:=nil;
  if FormInMemoryFirst then
    FStream:=TMemoryStream.Create()
  else
    FFile := CreateFileW(PChar(FileName),GENERIC_WRITE,0,nil,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL or FILE_FLAG_WRITE_THROUGH,0);
end;

destructor TSimpleWriteFile.Destroy;
begin
  Flush();
  if Assigned(FStream) then
    FStream.Free()
  else
    CloseHandle(FFile);
  inherited;
end;

procedure TSimpleWriteFile.Flush;
begin
  if Assigned(FStream) then
    FStream.SaveToFile(FFileName)
  else
    FlushFileBuffers(FFile);
end;

procedure TSimpleWriteFile.Write(F: Pointer; WriteSize: integer);
var
  i:cardinal;
begin
  if Assigned(FStream) then
    FStream.Write(F^,WriteSize)
  else
  	Windows.WriteFile(FFile,F^,WriteSize,i,nil);
end;

{ TSimpleWriteFileBase }

procedure TSimpleWriteFileBase.WriteBoolean(b: boolean);
var
  bt:byte;
begin
  if b then
    bt:=1
  else
    bt:=0;
  Write(@bt,1);
end;

procedure TSimpleWriteFileBase.WriteByte(b: Byte);
begin
  Write(@b,1);
end;

procedure TSimpleWriteFileBase.WriteDelphiString(const s: ansistring);
begin
  WriteInteger(Length(s));
  Write(@s[1],Length(s));
end;

procedure TSimpleWriteFileBase.WriteFloat(f: single);
begin
  Write(@f,sizeof(f));
end;

procedure TSimpleWriteFileBase.WriteInteger(i: integer);
begin
  Write(@i,sizeof(i));
end;

procedure TSimpleWriteFileBase.WriteNullTerminatedString(const s: ansistring);
var
  c:ansichar;
begin
  //for i := 1 to Length(s) do
  //  Write(@s[i],1);
  Write(@s[1],Length(s));
  c:=#0;
  Write(@c,1);
end;



{ TSimpleWriteFileBlockMap }

constructor TSimpleWriteFileBlockMap.Create(FileName: string;
  BlockSize: integer);
begin
  FFileName:=FileName;

  FFile := CreateFileW(PChar(FileName),GENERIC_WRITE,0,nil,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL or FILE_FLAG_WRITE_THROUGH,0);
  FDataMaxSize:=BlockSize;
  FData:=GetMemory(FDataMaxSize);
  FDataCurrentSize:=0;
end;

destructor TSimpleWriteFileBlockMap.Destroy;
begin
  Flush();
  FreeMemory(FData);
  CloseHandle(FFile);
  inherited;
end;

procedure TSimpleWriteFileBlockMap.Dump;
var
  i:cardinal;
begin
  if FDataCurrentSize=0 then Exit;

  Windows.WriteFile(FFile,FData^,FDataCurrentSize,i,nil);
  FDataCurrentSize:=0;
end;

procedure TSimpleWriteFileBlockMap.Flush;
begin
  Dump();
  FlushFileBuffers(FFile);
end;

procedure TSimpleWriteFileBlockMap.Write(F: Pointer; WriteSize: integer);
var
  i:cardinal;
begin
  if FDataMaxSize<WriteSize then begin //Если данные больше буффера, то скидываем буффер и пишем данные напрямую
    Dump();
    Windows.WriteFile(FFile,F^,WriteSize,i,nil);
    Exit;
  end;

  if FDataCurrentSize+WriteSize>FDataMaxSize then //Если для данных не хватаем места в буффере - сбрасываем буффер
    Dump();

  CopyMemory(Pointer(NativeUInt(FData)+FDataCurrentSize), F, WriteSize);
  inc(FDataCurrentSize,WriteSize);
end;

end.

