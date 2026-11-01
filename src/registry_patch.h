/* Experimental private native Epoch explosion; guarded build-specific table extension. */
static uint32_t nxSourceRva=0x37CC920u,nxSourceCount=423u;
#define NX_TABLE nxSourceRva
#define NX_READER 0x13C0DC0u
#define NX_BOUND 0x13C0DB5u
#define NX_NATIVE nxSourceCount
#define NX_PRIVATE nxSourceCount
#define NX_COUNT (nxSourceCount+64u)
#define NX_MAX_PATCHES 192u

typedef struct NxPatch {
    uintptr_t at;
    uint32_t before, after;
} NxPatch;
static NxPatch nxPatches[NX_MAX_PATCHES];
static unsigned nxPatchCount;
static uint8_t* nxBlock;
static int nxReady;

static const char* nxFailure="not attempted";
static uintptr_t nxFailureRva;

static int NxFail(const char* reason,uintptr_t rva)
{ nxFailure=reason;nxFailureRva=rva;return 0; }

static int NxAddPatch(uintptr_t at,uint32_t before,uint32_t after)
{
    for (unsigned i=0;i<nxPatchCount;i++)
        if (nxPatches[i].at==at) return nxPatches[i].before==before && nxPatches[i].after==after;
    if (nxPatchCount>=NX_MAX_PATCHES) return 0;
    nxPatches[nxPatchCount++]=(NxPatch){at,before,after};
    return 1;
}

/* Supported x64 pointer-table address forms are the same ones used by
 * HellForge's projectile registry: image-relative indexed disp32, absolute
 * indexed RVA, and RIP-relative LEA/MOV of the table or a fixed pointer slot.
 * Every displacement is decoded, range/alignment checked and read back before
 * writing. A raw integer occurrence without a matching instruction is ignored.
 */
static int NxScan(const uint8_t* code,uint32_t first,uint32_t size,uintptr_t table,
                  unsigned* refs,unsigned* bounds)
{
    uintptr_t locations[NX_MAX_PATCHES];
    unsigned count=0;
    for (uint32_t i=0;i+8<=size;i++)
    {
        const uint8_t* p=code+i;
        if ((p[0]&0xf8)!=0x48) continue;
        unsigned immediate=0;
        if (p[1]==0x83 && (p[2]&0x38)==0x38) immediate=1;
        else if ((p[1]==0x81 && (p[2]&0x38)==0x38) ||
                 (p[1]==0xc7 && !(p[2]&0x38))) immediate=4;
        else if (p[1]!=0x8b && p[1]!=0x8d && p[1]!=0x89) continue;
        unsigned dispAt=0,length=0;
        uintptr_t oldTarget=0,newTarget=0;
        uint32_t disp=0;
        if ((p[2]&0xc7)==0x84 || ((p[2]&0xc7)==0x04 && (p[3]&7)==5))
        {
            /* indexed pointers: scale 8 (or LEA of a fixed RVA with no index). */
            if ((p[3]>>6)!=3 && !((p[3]&0x38)==0x20 && !(p[0]&2))) continue;
            memcpy(&disp,p+4,4);
            if (disp<NX_TABLE || disp>=NX_TABLE+NX_NATIVE*8 || (disp-NX_TABLE)%8) continue;
            dispAt=4; length=8+immediate;
            oldTarget=gameBase+disp;
            newTarget=table+(disp-NX_TABLE);
        }
        else if ((p[2]&0xc7)==0x05)
        {
            memcpy(&disp,p+3,4);
            length=7+immediate;
            oldTarget=gameBase+first+i+length+(int32_t)disp;
            if (oldTarget<gameBase+NX_TABLE || oldTarget>=gameBase+NX_TABLE+NX_NATIVE*8 ||
                (oldTarget-gameBase-NX_TABLE)%8) continue;
            dispAt=3;
            newTarget=table+(oldTarget-gameBase-NX_TABLE);
        }
        else if ((p[2]&0xc0)==0x80 && (p[2]&7)!=4)
        {
            /* Fixed slot addressed through an image-base register: MOV,
             * LEA, CMP and initializer stores need relocation too. */
            memcpy(&disp,p+3,4);
            if (disp<NX_TABLE || disp>=NX_TABLE+NX_NATIVE*8 || (disp-NX_TABLE)%8) continue;
            dispAt=3;length=7+immediate;
            oldTarget=gameBase+disp;
            newTarget=table+(disp-NX_TABLE);
        }
        if (!dispAt) continue;
        if (i+length>size) continue;
        if (count>=NX_MAX_PATCHES) return NxFail("too many explosion table references",first+i);
        locations[count++]=gameBase+first+i;
        int rip=(p[2]&0xc7)==0x05;
        int64_t adjusted=rip?(int64_t)newTarget-(int64_t)(gameBase+first+i+length):
            (int64_t)newTarget-(int64_t)gameBase;
        if (adjusted<INT32_MIN || adjusted>INT32_MAX ||
            !NxAddPatch(gameBase+first+i+dispAt,disp,(uint32_t)(int32_t)adjusted)) return 0;
    }
    /* Raise only type bounds beside an explosion-table reader, not unrelated
     * occurrences of the integer 423 elsewhere in the image. */
    for (uint32_t i=0;i+7<=size;i++)
    {
        const uint8_t* p=code+i;
        unsigned off=p[0]==0x3d?1:(p[0]==0x81 && (p[1]&0xf8)==0xf8?2:0);
        if (!off) continue;
        uint32_t value;memcpy(&value,p+off,4);
        if (value!=NX_NATIVE) continue;
        uintptr_t at=gameBase+first+i;
        int adjacent=0;
        for (unsigned k=0;k<count;k++)
            if (at<=locations[k] && locations[k]-at<=128) { adjacent=1;break; }
        if (adjacent)
        {
            if (!NxAddPatch(at+off,value,NX_COUNT)) return 0;
            (*bounds)++;
        }
    }
    /* Do not silently leave an unrecognised image-relative table reference
     * pointing at the smaller array. This also rejects a competing patch
     * that changed the surrounding addressing instruction. */
    for (uint32_t i=0;i+4<=size;i++)
    {
        uint32_t value;memcpy(&value,code+i,4);
        if (value<NX_TABLE || value>=NX_TABLE+NX_NATIVE*8 || (value-NX_TABLE)%8) continue;
        int found=0;
        for (unsigned k=0;k<nxPatchCount;k++)
            if (nxPatches[k].at==gameBase+first+i) { found=1;break; }
        if (!found) return NxFail("unsupported native explosion table instruction",first+i);
    }
    *refs+=count;
    return 1;
}

/* Captured 0.1.1 code proves the kick caches the table in RDX BEFORE its
 * CMP EAX,423. The heuristic only found bounds before a reader, missing this
 * guard. Pin this exact reader/branch sequence rather than widening a scan. */
static int NxPlanKickBound(const HfApi* hf)
{
 const uint32_t boundRva=0x13c551e,tableRva=0x13c54df;
 const uint8_t tail[]={0x73,0x08,0x85,0xc0,0x74,0x04,0x4c,0x8b,0x34,0xc2};
 const uint8_t cmp[]={0x3d},lea[]={0x48,0x8d,0x15};
 uint32_t count;int32_t disp;
 if(!CodeIs(hf,gameBase+boundRva,cmp,1)||!CodeIs(hf,gameBase+boundRva+5,tail,sizeof tail)||
    !CodeIs(hf,gameBase+tableRva,lea,3)||hf->read(gameBase+boundRva+1,&count,4)||
    hf->read(gameBase+tableRva+3,&disp,4)||count!=NX_NATIVE||
    gameBase+tableRva+7+disp!=gameBase+NX_TABLE)return NxFail("kick-stage guard or table reference differs",boundRva);
 return NxAddPatch(gameBase+boundRva+1,count,NX_COUNT);
}

static int NxBuildRows(const HfApi* hf,uintptr_t pointers[NX_NATIVE])
{
    uintptr_t table=nxReady?(uintptr_t)nxBlock:gameBase+NX_TABLE;
    if (hf->read(table,pointers,NX_NATIVE*sizeof *pointers)) return NxFail("explosion table not readable",NX_TABLE);
    for (unsigned i=1;i<NX_NATIVE;i++)
    {
        uint32_t id;
        if (!pointers[i] || hf->read(pointers[i],&id,4) || id!=i) return NxFail("native explosion row not ready",i);
    }

    return 1;
}

static int NxReadCode(const HfApi* hf,uintptr_t at,uint8_t* copy,uint32_t length)
{
    /* Reads crossing a protected page can fail although the needed code is
     * readable. Bound requests and retry a failed block page by page. Never
     * scan an unreadable page as though its bytes were zero. */
    for (uint32_t off=0;off<length;)
    {
        uint32_t amount=length-off<32768?length-off:32768;
        if (hf->read(at+off,copy+off,amount))
            for (uint32_t page=0;page<amount;)
            {
                uint32_t size=amount-page<4096?amount-page:4096;
                if (hf->read(at+off+page,copy+off+page,size)) return NxFail("native code page not readable",at+off+page-gameBase);
                page+=size;
            }
        off+=amount;
    }
    return 1;
}

static int NxInstall(HfMod* mod)
{
    const HfApi* hf=HF_API(mod);
    size_t image=0;
    gameBase=hf->module_base("game.dll",&image);
    if (!gameBase || image!=IMAGE_SIZE) return NxFail("unsupported game image",0);
    const uint8_t reader[]={0x4c,0x8b,0xb4,0xc1};
    const uint8_t bound[]={0x3d};
    if (!CodeIs(hf,gameBase+NX_READER,reader,4)) return NxFail("native explosion reader not decoded",NX_READER);
    if (!CodeIs(hf,gameBase+NX_BOUND,bound,1)) return NxFail("native explosion bound not decoded",NX_BOUND);
    uint32_t rva=0,count=0;
    if(hf->read(gameBase+NX_READER+4,&rva,4) || hf->read(gameBase+NX_BOUND+1,&count,4) ||
       count!=423 || rva<0x1000 || rva>0x70000000u-count*8)
        return NxFail("unsupported existing explosion table",NX_READER);
    /* This provider owns the extension. Refuse an existing external extension;
     * cooperating consumers must share this registry rather than patch again. */
    nxSourceRva=rva;nxSourceCount=count;
    uintptr_t pointers[NX_NATIVE];
    if (!NxBuildRows(hf,pointers)) return 0;
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS64 nt;
    if (hf->read(gameBase,&dos,sizeof dos) || dos.e_magic!=IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew<0 || (uint32_t)dos.e_lfanew>IMAGE_SIZE-sizeof nt ||
        hf->read(gameBase+dos.e_lfanew,&nt,sizeof nt) || nt.Signature!=IMAGE_NT_SIGNATURE ||
        nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt.OptionalHeader.SizeOfImage!=IMAGE_SIZE || nt.FileHeader.NumberOfSections>32) return NxFail("game PE headers not ready",0);
    uint8_t* block=AllocateNear(gameBase,16384);
    if (!block) return NxFail("could not allocate native table near game",0);
    memcpy(block,pointers,sizeof pointers);
    uintptr_t* table=(uintptr_t*)block;
    for(unsigned i=0;i<64;i++)table[NX_PRIVATE+i]=0;
    nxPatchCount=0;
    unsigned refs=0,bounds=0;
    uintptr_t sections=gameBase+dos.e_lfanew+24+nt.FileHeader.SizeOfOptionalHeader;
    int valid=1;
    nxFailure="native table census incomplete";nxFailureRva=0;
    for (unsigned s=0;s<nt.FileHeader.NumberOfSections && valid;s++)
    {
        IMAGE_SECTION_HEADER sh;
        if (hf->read(sections+s*sizeof sh,&sh,sizeof sh)) { valid=0;break; }
        if (!(sh.Characteristics&IMAGE_SCN_MEM_EXECUTE) || sh.VirtualAddress>=0x2111000u) continue;
        uint32_t length=sh.Misc.VirtualSize;
        if (sh.VirtualAddress>=IMAGE_SIZE || !length || length>IMAGE_SIZE-sh.VirtualAddress) { valid=0;break; }
        uint8_t* copy=VirtualAlloc(NULL,length,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        if (!copy) { valid=0;break; }
        if (!NxReadCode(hf,gameBase+sh.VirtualAddress,copy,length) ||
            !NxScan(copy,sh.VirtualAddress,length,(uintptr_t)block,&refs,&bounds)) valid=0;
        VirtualFree(copy,0,MEM_RELEASE);
    }
    if(valid&&!NxPlanKickBound(hf))valid=0;
    /* The primary reader and its bound must be among the decoded candidates.
     * All validation happens before the first code write. */
    int primary=0,primaryBound=0;
    for (unsigned i=0;i<nxPatchCount;i++)
    {
        primary|=nxPatches[i].at==gameBase+NX_READER+4;
        primaryBound|=nxPatches[i].at==gameBase+NX_BOUND+1;
        uint32_t current;
        if (hf->read(nxPatches[i].at,&current,4) || current!=nxPatches[i].before) valid=0;
    }
    if (!valid || !primary || !primaryBound || !refs || !bounds)
    { VirtualFree(block,0,MEM_RELEASE);return 0; }
    unsigned applied=0;
    for (;applied<nxPatchCount;applied++)
    {
        uint32_t verify=0;
        if (hf->write(nxPatches[applied].at,&nxPatches[applied].after,4) ||
            hf->read(nxPatches[applied].at,&verify,4) || verify!=nxPatches[applied].after) break;
        FlushInstructionCache(GetCurrentProcess(),(void*)nxPatches[applied].at,4);
    }
    if (applied!=nxPatchCount)
    {
        int rollback=1;
        /* Include the failed attempt: an API error can follow a partial write. */
        applied++;
        while (applied)
        {
            --applied;
            uint32_t verify=0;
            if (hf->write(nxPatches[applied].at,&nxPatches[applied].before,4) ||
                hf->read(nxPatches[applied].at,&verify,4) || verify!=nxPatches[applied].before) rollback=0;
            FlushInstructionCache(GetCurrentProcess(),(void*)nxPatches[applied].at,4);
        }
        if (rollback) VirtualFree(block,0,MEM_RELEASE);
        else nxBlock=block; /* a remaining reader may still name the block */
        hf->log(mod,"Native plasma explosion extension refused; rollback %s",rollback?"complete":"incomplete; storage retained");
        return 0;
    }
    nxBlock=block;nxReady=1;nxFailure="ready";nxFailureRva=0;
    hf->log(mod,"Explosives Library registry starts at %u: %u references, %u scanned bounds (+ explicit kick guard)",NX_PRIVATE,refs,bounds);
    return 1;
}
