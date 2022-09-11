#pragma once
#ifndef SUBPAGE_H
#define SUBPAGE_H

#include "FlashTypes.h"
#include "Page.h"

namespace NVM
{
	namespace FlashMemory
	{
		
		struct SubPageMetadata
		{
			//page_status_type Status;
			LPA_type LPA;
		};

				
		class SubPage {
		public:
			SubPage()
			{
				//Metadata.Status = FREE_PAGE;
				Metadata.LPA = NO_LPA;
				//Metadata.SourceStreamID = NO_STREAM;
			};

			SubPageMetadata Metadata;
			//PageMetadata Metadata_p;
			
			void Write_metadata(const SubPageMetadata& metadata)
			{
				this->Metadata.LPA = metadata.LPA;
			}

			/*
			void Write_metadata_p(const PageMetadata& metadata_p)
			{
				this->Metadata.LPA = metadata_p.LPA;
			}
			
			*/
			
			void Read_metadata(SubPageMetadata& metadata)
			{
				metadata.LPA = this->Metadata.LPA;
				if (this->Metadata.LPA == 283963) {
					////std::cout << "[DEBUG GC-Read_metadata()] read LPA = 283963 from chip "<< std::endl;
				}
			}
		};
	}
}

#endif // !SUBPAGE_H
