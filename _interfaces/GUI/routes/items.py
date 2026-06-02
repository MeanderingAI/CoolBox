from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse, FileResponse
from sqlmodel import SQLModel, Field, Session, create_engine, select
from typing import Optional, List
import os

router = APIRouter()

class Item(SQLModel, table=True):
    id: Optional[int] = Field(default=None, primary_key=True)
    name: str
    description: Optional[str] = None

engine = create_engine("sqlite:///./dashboard.db", echo=True)

@router.on_event("startup")
def on_startup():
    SQLModel.metadata.create_all(engine)

@router.post("/items/", response_model=Item)
def create_item(item: Item):
    with Session(engine) as session:
        session.add(item)
        session.commit()
        session.refresh(item)
        return item

@router.get("/items/", response_model=List[Item])
def read_items():
    with Session(engine) as session:
        items = session.exec(select(Item)).all()
        return items
